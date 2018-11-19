
// PushStreamToolDlg.cpp : ʵ���ļ�
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


// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// �Ի�������
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
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


// CPushStreamToolDlg �Ի���



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


// CPushStreamToolDlg ��Ϣ�������

BOOL CPushStreamToolDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
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

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������
	//�򿪿���̨
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
	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
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

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CPushStreamToolDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CPushStreamToolDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CPushStreamToolDlg::OnBnClickedButtonOpenfile()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	if(openSourceFileStatus){
		MessageBox(_T("����ֹͣ��ǰ�򿪵��ļ�"));
		return;
	}
	CString szFilter = _T("All Files (*.*)|*.*|avi Files (*.avi)|*.avi|rmvb Files (*.rmvb)|*.rmvb|3gp Files (*.3gp)|*.3gp|mp3 Files (*.mp3)|*.mp3|mp4 Files (*.mp4)|*.mp4|mpeg Files (*.ts)|*.ts|flv Files (*.flv)|*.flv|mov Files (*.mov)|*.mov||");
	CFileDialog OpenDlg(TRUE,NULL ,NULL,OFN_PATHMUSTEXIST|OFN_HIDEREADONLY ,szFilter,NULL); 
	if(IDOK == OpenDlg.DoModal()){
		//����Դ�ļ�
		m_SourceFileName = OpenDlg.GetPathName();
		pushStatus = 0;
		//�༭����ʾ�ļ�·��
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
		sprintf_s(sdl_var, "SDL_WINDOWID=%d", hWnd); //����һ�������пո�SDL_WINDOWID=%d"
		SDL_putenv(sdl_var);
		char *myvalue = SDL_getenv("SDL_WINDOWID");//��SDL��ȡ����ID
	}
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		printf("���ܳ�ʼ��SDL--%s\n", SDL_GetError());
		return -1;
	}
	//����SDL�¼�״̬
	SDL_EventState(SDL_ACTIVEEVENT, SDL_IGNORE);
	SDL_EventState(SDL_SYSWMEVENT, SDL_IGNORE);
	SDL_EventState(SDL_USEREVENT, SDL_IGNORE);
	//��ȡStatic�ؼ��Ŀ�Ⱥ͸߶�
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
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	this->GetDlgItem(IDC_EDIT_Source)->GetWindowText(this->m_SourceFileName);
	this->GetDlgItem(IDC_EDIT_URL)->GetWindowText(m_URL);
	if(m_URL == ""){
		MessageBox(_T("������URL"));
		return;
	}
	CString str;
	this->GetDlgItem(IDC_BUTTON_StartPush)->GetWindowText(str);
	if(str == "��ʼ����"){
		if(m_radio_ffmpeg.GetCheck()){
			//���������߳�
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
		this->GetDlgItem(IDC_BUTTON_StartPush)->SetWindowText("ֹͣ����");
	}else{
		this->pushStatus = 0;
		this->GetDlgItem(IDC_BUTTON_StartPush)->SetWindowText("��ʼ����");
		//д�ļ�β��Write file trailer��
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
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	this->GetDlgItem(IDC_EDIT_URL)->GetWindowText(m_URL);
	if(m_URL == ""){
		MessageBox(_T("������URL"));
		return;
	}
	CString cstr;
	this->GetDlgItem(IDC_BUTTON_Encode)->GetWindowTextA(cstr);
	if(cstr == "��ʼ����"){
		HANDLE encodeThread;
		DWORD encodeThreadID;
		this->encodeStatus = 1;
		encodeThread = CreateThread(NULL, 0,(LPTHREAD_START_ROUTINE)EncodeFun, this, 0, &encodeThreadID);
		if(NULL == encodeThread){
			MessageBox(_T("create encodeThread failed"));
		    this->encodeStatus = 0;
			return;
		}
		this->GetDlgItem(IDC_BUTTON_Encode)->SetWindowText("ֹͣ����");
	}else{
		this->encodeStatus = 0;
		this->GetDlgItem(IDC_BUTTON_Encode)->SetWindowTextA("��ʼ����");
	}
}


void CPushStreamToolDlg::OnBnClickedButtonPause()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	
	CString cstr;
	this->GetDlgItem(IDC_BUTTON_Pause)->GetWindowTextA(cstr);
	if(cstr == "��ͣ"){
		pushStatus = 2;
		this->GetDlgItem(IDC_BUTTON_Pause)->SetWindowText("����");
	}else{
		pushStatus = 1;
		this->GetDlgItem(IDC_BUTTON_Pause)->SetWindowTextA("��ͣ");
	}
}

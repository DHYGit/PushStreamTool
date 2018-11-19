
// PushStreamToolDlg.h : ͷ�ļ�
//

#pragma once
#include "afxwin.h"
#include "FFmpegTool.h"

// CPushStreamToolDlg �Ի���
class CPushStreamToolDlg : public CDialogEx
{
// ����
public:
	CPushStreamToolDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_PUSHSTREAMTOOL_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��
public:

	CString m_SourceFileName;
	CString m_URL;
	int openSourceFileStatus;
	int pushStatus;//1.����������2ֹͣ����
	int encodeStatus;
	FFmpegClass *ffmpeg;

	int InitFFmpeg();
	int InitSDL();
	int static_width;
	int static_height;
	
	HANDLE PushThread;
	DWORD PushThreadID;
	
// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonOpenfile();
	afx_msg void OnBnClickedButtonStartpush();
	CButton m_radio_ffmpeg;
	afx_msg void OnBnClickedButtonEncode();
	afx_msg void OnBnClickedButtonPause();
	CButton m_ratio_rtmp;
	CButton m_radio_hls;
};

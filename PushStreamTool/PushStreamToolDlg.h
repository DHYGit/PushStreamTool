
// PushStreamToolDlg.h : 头文件
//

#pragma once
#include "afxwin.h"
#include "FFmpegTool.h"

// CPushStreamToolDlg 对话框
class CPushStreamToolDlg : public CDialogEx
{
// 构造
public:
	CPushStreamToolDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_PUSHSTREAMTOOL_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持
public:

	CString m_SourceFileName;
	CString m_URL;
	int openSourceFileStatus;
	int pushStatus;//1.进行推流，2停止推流
	int encodeStatus;
	FFmpegClass *ffmpeg;

	int InitFFmpeg();
	int InitSDL();
	int static_width;
	int static_height;
	
	HANDLE PushThread;
	DWORD PushThreadID;
	
// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
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

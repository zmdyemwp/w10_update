//---------------------------------------------------------------------------

#include <vcl.h>
#include <objbase.h>
#pragma hdrstop

#include "Unit1.h"
#include "ymodem.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
const TCHAR       START_UPDATE_CONST[] = TEXT("START_UPDATE_CONST");
TForm1 *Form1;
//---------------------------------------------------------------------------
__fastcall TForm1::TForm1(TComponent* Owner)
        : TForm(Owner)
{
     hCom = INVALID_HANDLE_VALUE;
     Thread_Init();
     MessageInit();
}
//---------------------------------------------------------------------------
void   __fastcall TForm1::Thread_Init(void){
     START_UPDATE_EVENT  = CreateEvent(NULL, FALSE, FALSE, START_UPDATE_CONST);
     SystemThread=new TSystemThread();
     SystemThread->FreeOnTerminate=true;
}

//---------------------------------------------------------------------------
void   __fastcall  TForm1::Thread_Close(void){
     if(SystemThread!=NULL){
        SystemThread->Terminate();
     }
}
//---------------------------------------------------------------------------
_fastcall TSystemThread::TSystemThread(void):TThread(true){
     FreeOnTerminate=true;
     Resume();
}
//---------------------------------------------------------------------------

void __fastcall TForm1::FormClose(TObject *Sender, TCloseAction &Action)
{
     if(SystemThread!=NULL){
        Action = (TCloseAction)0;
        Thread_Close();
        CloseTimer->Enabled = true;
        return;
     }

}
//---------------------------------------------------------------------------
void __fastcall TForm1::CloseTimerTimer(TObject *Sender)
{
     CloseTimer->Enabled = false;
     SystemThread = NULL;
     CloseHandle(START_UPDATE_EVENT);
     Close();

}
//---------------------------------------------------------------------------
void TForm1::MessageInit(void){
     Message_Max_Text_Size = Memo1->MaxLength - 100;
     Message_Counter = 0;
     SendDlgItemMessage (Handle, GetDlgCtrlID(Memo1->Handle), EM_LIMITTEXT, Message_Max_Text_Size, 0);
     MessageClear();
}
//---------------------------------------------------------------------------
void TForm1::ShowTestMessage(char *format, ...){
     char                                 data[16384];
     va_list                                   arg_pt;
     DWORD                                        len;
     va_start(arg_pt, format);
     if(format == NULL){
     	format = "(null)";
     }
     wvsprintf(data,format,arg_pt);
     len = strlen(data);
     if((Message_Counter+len)>Message_Max_Text_Size){
    	SendMessage (Memo1->Handle, EM_SETSEL, (WPARAM)0, (LPARAM)(len));
    	SendDlgItemMessage (Handle, GetDlgCtrlID(Memo1->Handle), EM_REPLACESEL, (WPARAM)TRUE, (LPARAM)TEXT(""));
    	Message_Counter =(Message_Counter - len);
     }
     SendMessage (Memo1->Handle, EM_SETSEL, (WPARAM)Message_Counter, (LPARAM)(Message_Counter+len));
     SendDlgItemMessage (Handle, GetDlgCtrlID(Memo1->Handle), EM_REPLACESEL, 0, (LPARAM)data);
     Message_Counter+=len;
}

//---------------------------------------------------------------------------
void TForm1::MessageClear(void){
     SendMessage (Memo1->Handle, EM_SETSEL, (WPARAM)0, (LPARAM)(Message_Counter));
     SendDlgItemMessage (Handle, GetDlgCtrlID(Memo1->Handle), EM_REPLACESEL, 0, (LPARAM)TEXT(""));
     Message_Counter = 0;
}
//---------------------------------------------------------------------------
void __fastcall TSystemThread::Execute(){
     DWORD dwWaitStatus, dwTimeout = 3000;
     while(!Terminated){
	   dwWaitStatus = WaitForMultipleObjects(1, &Form1->START_UPDATE_EVENT, FALSE, dwTimeout);
	   if(dwWaitStatus == WAIT_OBJECT_0 + 0){
              CoInitialize(NULL);
              Form1->Process_Update();
              CoUninitialize();
           }
     }
}
//---------------------------------------------------------------------------
#define  UART_COM_BYTE                                     8
#define  UART_COM_PARITY                                   NOPARITY
#define  UART_COM_STOPBIT                                  ONESTOPBIT

bool TForm1::UART_OPEN(DWORD index){
     char                   buf[100];
     COMMTIMEOUTS               time;
	 if(hCom!=INVALID_HANDLE_VALUE){
        return TRUE;
     }
     wsprintf(buf,TEXT("\\\\.\\COM%d\\"),index);
     hCom=CreateFile(buf,GENERIC_READ | GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
     if(hCom==INVALID_HANDLE_VALUE){
        return FALSE;
     }
     if(!GetCommTimeouts(hCom,&time)){
        CloseHandle(hCom);
        hCom = INVALID_HANDLE_VALUE;
        return FALSE;
     }
     time.ReadIntervalTimeout=MAXDWORD;
     time.ReadTotalTimeoutMultiplier=0;
     time.ReadTotalTimeoutConstant=0;
     time.WriteTotalTimeoutMultiplier=0;
     time.WriteTotalTimeoutConstant=0;
     if(!SetCommTimeouts(hCom,&time)){
        CloseHandle(hCom);
        hCom = INVALID_HANDLE_VALUE;
        return FALSE;
     }
     return TRUE;
}

void TForm1::UART_CLOSE(void){
     if(hCom!=INVALID_HANDLE_VALUE){
        CloseHandle(hCom);
        hCom = INVALID_HANDLE_VALUE;
     }
}

int TForm1::UART_SET_BAUDRATE(DWORD baud){
     DCB                      dcb;
     if(hCom==INVALID_HANDLE_VALUE){
        return FALSE;
     }
     if(!GetCommState(hCom,&dcb)){
        CloseHandle(hCom);
        hCom = INVALID_HANDLE_VALUE;
        return FALSE;
     }
     dcb.fRtsControl  = RTS_CONTROL_ENABLE;
     dcb.fOutxCtsFlow = TRUE;
     dcb.fDtrControl  = DTR_CONTROL_DISABLE;
     dcb.fOutxDsrFlow = FALSE;
     dcb.fOutX        = FALSE;
     dcb.fInX         = FALSE;
     dcb.BaudRate     = baud;
     dcb.ByteSize     = UART_COM_BYTE;
     dcb.Parity       = UART_COM_PARITY;
     dcb.StopBits     = UART_COM_STOPBIT;
     if(!SetCommState(hCom,&dcb)){
        CloseHandle(hCom);
        hCom = INVALID_HANDLE_VALUE;
        return FALSE;
     }
     return 0;
}

int TForm1::UART_READ(char *dataptr,DWORD bytes_to_read,DWORD *len){
     DWORD length;
     length = bytes_to_read;
     if(!ReadFile(hCom,dataptr,length,&length,NULL)){
        CloseHandle(hCom);
        hCom = INVALID_HANDLE_VALUE;
        return 0;
     }
     *len =length;
     return 1;
}

int TForm1::UART_WRITE(char *dataptr,DWORD bytes_to_write,DWORD *len){
     DWORD length;
     length = bytes_to_write;
     if(!WriteFile(hCom,dataptr,length,&length,NULL)){
        CloseHandle(hCom);
        hCom = INVALID_HANDLE_VALUE;
        return 0;
     }
     *len = length;
     return 1;
}

//---------------------------------------------------------------------------
void __fastcall TForm1::Button1Click(TObject *Sender)
{
    HANDLE file;
    DWORD length,index,readlen;
    Button1->Enabled = false;
    ComboBox1->Enabled = false;
    if(!OpenDialog1->Execute()){
       Button1->Enabled = true;
       ComboBox1->Enabled = true;
       return;
    }
    MessageClear();
    ProgressBar1->Position = 0;
    ShowTestMessage("Read %s\r\n",OpenDialog1->FileName.c_str());
    Label2->Caption = OpenDialog1->FileName;
    length = strlen(OpenDialog1->FileName.c_str());
    memset(filename,0,256);
    if(length > 256){
       ShowTestMessage("Filename length error %d > 256\r\n",length);
       Button1->Enabled = true;
       ComboBox1->Enabled = true;
       return;
    }
    memcpy(filename,OpenDialog1->FileName.c_str(),length);
    memset(Image_data,0,0x200000);
    file = CreateFile(OpenDialog1->FileName.c_str(),GENERIC_READ | GENERIC_WRITE,0,NULL,OPEN_EXISTING,0,NULL);
    if(file==INVALID_HANDLE_VALUE){
       ShowTestMessage("File open error\r\n");
       Button1->Enabled = true;
       ComboBox1->Enabled = true;
       return;
    }
    length = GetFileSize(file, NULL);
    Image_length = length;
    if(length > USER_FLASH_SIZE){
       CloseHandle(file);
       ShowTestMessage("File size error %d > %d\r\n",length,USER_FLASH_SIZE);
       Button1->Enabled = true;
       ComboBox1->Enabled = true;
       return;
    }
    for(index=0;index<length;){
    	readlen = (length - index);
        if(!ReadFile(file,Image_data+index,readlen,&readlen,NULL)){
           ShowTestMessage("Read file error\r\n");
           CloseHandle(file);
           Button1->Enabled = true;
           ComboBox1->Enabled = true;
           return;
        }
        index +=readlen;
    }
    CloseHandle(file);
    SetEvent(START_UPDATE_EVENT);
}
//---------------------------------------------------------------------------
void TForm1::Process_Update(void){
    char        buffer[256];
    DWORD         in_length;
    DWORD        out_length;
    DWORD                 i;
    DWORD      total_length;
    buffer[0] = 'a';
    buffer[1] = '0';
    in_length = 2;
    ShowTestMessage("OPEN %s .....\r\n",ComboBox1->Text.c_str());
    if(!UART_OPEN(ComboBox1->ItemIndex + 1)){
       ShowTestMessage("OPEN %s ERROR\r\n",ComboBox1->Text.c_str());
       Button1->Enabled = true;
       ComboBox1->Enabled = true;
       return;
    }
    ShowTestMessage("OPEN %s OK\r\n",ComboBox1->Text.c_str());
    if(!UART_WRITE(buffer,in_length,&out_length)){
       ShowTestMessage("WRITE %s ERROR\r\n",ComboBox1->Text.c_str());
       UART_CLOSE();
       Button1->Enabled = true;
       ComboBox1->Enabled = true;
       return;
    }
    total_length = 0;
    for(i=0;i<2000;i++){
       memset(buffer,0,256);
       in_length = 128;
       if(!UART_READ(buffer,in_length,&out_length)){
          ShowTestMessage("READ %s ERROR\r\n",ComboBox1->Text.c_str());
          UART_CLOSE();
          Button1->Enabled = true;
          ComboBox1->Enabled = true;
          return;
       }
       ShowTestMessage("%s",buffer);
       total_length+=out_length;
       Sleep(1);
    }
    if(!total_length){
       ShowTestMessage("DEVICE NO RESPONDSE\r\n");
       UART_CLOSE();
       Button1->Enabled = true;
       ComboBox1->Enabled = true;
       return;
    }
    buffer[0] = '1';
    in_length = 1;
    if(!UART_WRITE(buffer,in_length,&out_length)){
       ShowTestMessage("WRITE %s ERROR\r\n",ComboBox1->Text.c_str());
       UART_CLOSE();
       Button1->Enabled = true;
       ComboBox1->Enabled = true;
       return;
    }
    total_length = 0;
    for(i=0;i<2000;i++){
       memset(buffer,0,256);
       in_length = 128;
       if(!UART_READ(buffer,in_length,&out_length)){
          ShowTestMessage("READ %s ERROR\r\n",ComboBox1->Text.c_str());
          UART_CLOSE();
          Button1->Enabled = true;
          ComboBox1->Enabled = true;
          return;
       }
       ShowTestMessage("%s",buffer);
       total_length+=out_length;
       Sleep(1);
    }
    if(!total_length){
       ShowTestMessage("DEVICE NO RESPONDSE\r\n");
       UART_CLOSE();
       Button1->Enabled = true;
       ComboBox1->Enabled = true;
       return;
    }
    if(!Ymodem_Transmit(Image_data,filename,Image_length)){
       for(i=0;i<2000;i++){
           memset(buffer,0,256);
           in_length = 128;
           if(!UART_READ(buffer,in_length,&out_length)){
              ShowTestMessage("READ %s ERROR\r\n",ComboBox1->Text.c_str());
              UART_CLOSE();
              Button1->Enabled = true;
              ComboBox1->Enabled = true;
              return;
           }
           ShowTestMessage("%s",buffer);
           total_length+=out_length;
           Sleep(1);
       }
    }
    buffer[0] = '3';
    in_length = 1;
    if(!UART_WRITE(buffer,in_length,&out_length)){
       ShowTestMessage("WRITE %s ERROR\r\n",ComboBox1->Text.c_str());
       UART_CLOSE();
       Button1->Enabled = true;
       ComboBox1->Enabled = true;
       return;
    }
    for(i=0;i<3000;i++){
       memset(buffer,0,256);
       in_length = 128;
       if(!UART_READ(buffer,in_length,&out_length)){
          ShowTestMessage("READ %s ERROR\r\n",ComboBox1->Text.c_str());
          UART_CLOSE();
          Button1->Enabled = true;
          ComboBox1->Enabled = true;
          return;
       }
       ShowTestMessage("%s",buffer);
       total_length+=out_length;
       Sleep(1);
    }
    UART_CLOSE();
    Button1->Enabled = true;
    ComboBox1->Enabled = true;
}


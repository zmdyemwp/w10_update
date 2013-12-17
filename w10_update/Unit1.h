//---------------------------------------------------------------------------

#ifndef Unit1H
#define Unit1H
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ComCtrls.hpp>
#include <Dialogs.hpp>
#include <ExtCtrls.hpp>
//---------------------------------------------------------------------------
class TSystemThread:public TThread{
    
__published:	// IDE-managed Components
    
private:	// User declarations
        void __fastcall Execute(void);
public:		// User declarations
         __fastcall TSystemThread(void);
};
//---------------------------------------------------------------------------
class TForm1 : public TForm
{
__published:	// IDE-managed Components
        TButton *Button1;
        TLabel *Label2;
        TProgressBar *ProgressBar1;
        TOpenDialog *OpenDialog1;
        TMemo *Memo1;
        TTimer *CloseTimer;
        TComboBox *ComboBox1;
        void __fastcall Button1Click(TObject *Sender);
        void   __fastcall                        Thread_Init(void);
        void   __fastcall                       Thread_Close(void);
        void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
        void __fastcall CloseTimerTimer(TObject *Sender);
private:	// User declarations
public:		// User declarations
        HANDLE                                        START_UPDATE_EVENT;
        TSystemThread                                      *SystemThread;
        HANDLE                                                      hCom;
        bool                                      UART_OPEN(DWORD index);
        void                                            UART_CLOSE(void);
        int                                UART_SET_BAUDRATE(DWORD baud);
        int      UART_READ(char *dataptr,DWORD bytes_to_read,DWORD *len);
        int    UART_WRITE(char *dataptr,DWORD bytes_to_write,DWORD *len);
        void                                        Process_Update(void);
        DWORD                                            Message_Counter;
        DWORD                                      Message_Max_Text_Size;
        void                                           MessageInit(void);
        void                          ShowTestMessage(char *format, ...);
        char                                        Image_data[0x200000];
        char                                               filename[256];
        unsigned int                                        Image_length;
        void                                          MessageClear(void);
        __fastcall TForm1(TComponent* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TForm1 *Form1;
//---------------------------------------------------------------------------
#endif

#include <stdio.h>
#include <string.h>
#include <ctype.h>

/**********************************************************************************************
**	Parallel port addresses
**********************************************************************************************/

#define PortA   *(volatile unsigned char *)(0x00400000)
#define PortB   *(volatile unsigned char *)(0x00400002)
#define PortC   *(volatile unsigned char *)(0x00400004)
#define PortD   *(volatile unsigned char *)(0x00400006)
#define PortE   *(volatile unsigned char *)(0x00400008)

#define TraceException  *(volatile unsigned char *)(0x0040000A)  // used to gnerate a trace exception for single stepping in the debug monitor

/*********************************************************************************************
**	Hex 7 seg displays port addresses
*********************************************************************************************/

#define HEX_A        *(volatile unsigned char *)(0x00400010)
#define HEX_B        *(volatile unsigned char *)(0x00400012)
#define HEX_C        *(volatile unsigned char *)(0x00400014)
#define HEX_D        *(volatile unsigned char *)(0x00400016)    //DE2 Only

/**********************************************************************************************
**	LCD display port addresses
**********************************************************************************************/

#define LCDcommand   *(volatile unsigned char *)(0x00400020)
#define LCDdata      *(volatile unsigned char *)(0x00400022)

/********************************************************************************************
**	Timer Port addresses
*********************************************************************************************/

#define Timer1Data      *(volatile unsigned char *)(0x00400030)
#define Timer1Control   *(volatile unsigned char *)(0x00400032)
#define Timer1Status    *(volatile unsigned char *)(0x00400032)

#define Timer2Data      *(volatile unsigned char *)(0x00400034)
#define Timer2Control   *(volatile unsigned char *)(0x00400036)
#define Timer2Status    *(volatile unsigned char *)(0x00400036)

#define Timer3Data      *(volatile unsigned char *)(0x00400038)
#define Timer3Control   *(volatile unsigned char *)(0x0040003A)
#define Timer3Status    *(volatile unsigned char *)(0x0040003A)

#define Timer4Data      *(volatile unsigned char *)(0x0040003C)
#define Timer4Control   *(volatile unsigned char *)(0x0040003E)
#define Timer4Status    *(volatile unsigned char *)(0x0040003E)

/*********************************************************************************************
**	RS232 port addresses
*********************************************************************************************/

#define RS232_Control     *(volatile unsigned char *)(0x00400040)
#define RS232_Status      *(volatile unsigned char *)(0x00400040)
#define RS232_TxData      *(volatile unsigned char *)(0x00400042)
#define RS232_RxData      *(volatile unsigned char *)(0x00400042)
#define RS232_Baud        *(volatile unsigned char *)(0x00400044)

/*********************************************************************************************
**	PIA 1 and 2 port addresses
*********************************************************************************************/

#define PIA1_PortA_DDR     *(volatile unsigned char *)(0x00400050)
#define PIA1_PortA_Control *(volatile unsigned char *)(0x00400052)
#define PIA1_PortB_DDR     *(volatile unsigned char *)(0x00400054)
#define PIA1_PortB_Control *(volatile unsigned char *)(0x00400056)

#define PIA2_PortA_DDR     *(volatile unsigned char *)(0x00400060)
#define PIA2_PortA_Control *(volatile unsigned char *)(0x00400062)
#define PIA2_PortB_DDR     *(volatile unsigned char *)(0x00400064)
#define PIA2_PortB_Control *(volatile unsigned char *)(0x00400066)

/*************************************************************
** SPI Controller registers
**************************************************************/
// SPI Registers
#define SPI_Control         (*(volatile unsigned char *)(0x00408020))
#define SPI_Status          (*(volatile unsigned char *)(0x00408022))
#define SPI_Data            (*(volatile unsigned char *)(0x00408024))
#define SPI_Ext             (*(volatile unsigned char *)(0x00408026))
#define SPI_CS              (*(volatile unsigned char *)(0x00408028))

// these two macros enable or disable the flash memory chip enable off SSN_O[7..0]
// in this case we assume there is only 1 device connected to SSN_O[0] so we can
// write hex FE to the SPI_CS to enable it (the enable on the flash chip is active low)
// and write FF to disable it

#define   Enable_SPI_CS()             SPI_CS = 0xFE
#define   Disable_SPI_CS()            SPI_CS = 0xFF 

// masks to extract various bits of the status register
#define SPSR_SPIF 0x80 // status register interrupt flag
#define SPSR_WCOL 0x40 // status register write collision flag
#define SPSR_WFFULL 0x08
#define SPSR_WFEMPTY 0x04
#define SPSR_RFFULL 0x02
#define SPSR_RFEMPTY 0x01

// masks for control register bits
#define SPCR_SPIE 0x80
#define SPCR_SPE 0x40
#define SPCR_MSTR 0x10
#define SPCR_CPOL 0x08
#define SPCR_CPHA 0x04
#define SPCR_SPR 0x03

// masks for ext register bits
#define SPER_ICNT 0xC0
#define SPER_ESPR 0x03

/*************************************************************
** Flash Commands
**************************************************************/
#define FLASH_ERASE_SECTOR 0x20
#define FLASH_READ_DATA 0x03
#define FLASH_PAGE_PROGRAM 0x02
#define FLASH_WRITE_ENABLE 0x06
#define FLASH_GET_STATUS_REGISTER1 0x05
#define FLASH_GET_MANUFACTURER_ID 0x90

/********************************************************************************************
**	RGB Colours
*********************************************************************************************/

#define RED     0x30
#define GREEN   0xC0
#define BLUE    0x20
#define WHITE   0xFF
#define BLACK   0

/********************************************************************************************
**	VideoRam addresses
*********************************************************************************************/

#define DramStart               0x08000000
#define DramEnd                 0x0BFFFFFF  // 64MB on DE1-soc
#define ProgramStart            0x08000000
#define ProgramEnd              0x0803FFFF  // 256Kbytes
#define Num_FlashSectors        ((ProgramEnd - ProgramStart)/65536)
#define FlashSize               (ProgramEnd - ProgramStart)
#define XRES			        640
#define YRES			        480
#define MemNumRows		        512
#define MemNumCols		        1024
#define XPIXELS			        7		// number of horizontal pixels in a column including space
#define YPIXELS			        9		// number of vertical pixels in a row including space
#define BorderHeight            4
#define BorderWidth		        4

/*******************************************************************************************
** Function Prototypes
*******************************************************************************************/

void Wait1ms(void);
void Wait3ms(void);
void Init_LCD(void) ;
void Outchar(int c);
void OutMess(char *theMessage);
void Clearln(void);
void Oline1(char *theMessage);
void Oline2(char *theMessage);
void PointXY(int x, int y, int colour);
void Block(int x, int y, int width, int height, int colour);
void PointXY(int x, int y, int colour);
void HLine(int x, int y, int length, int colour) ;
void VLine(int x, int y, int length, int colour) ;
int sprintf(char *out, const char *format, ...) ;

// SPI Prototypes
int  TestForSPITransmitData(void) ;
int  TestForWriteFifoEmpty(void);
int  ReadSPIChar(void);
void SPI_Init(void);
void WaitForSPITransmitComplete(void);
int  WriteSPIChar(int c);
void SetSPIFlashWriteEnableLatch(void);
void ClearSPIFlashWriteEnableLatch(void);
void WriteSPIFlashStatusReg(int Status);
int  ReadSPIFlashStatusReg(void);
void WaitForSPIFlashWriteCompletion(void);
void ReadSPIFlashData( int FlashAddress, unsigned char *MemoryAddress, int size);
int  ReadSPIFlashByte( int FlashAddress);
void SPIFlashProgram(int AddressOffset, int ByteData);
int  SPIFlashRead(int AddressOffset);
void WriteSPIFlashData(int FlashAddress, unsigned char *MemoryAddress, int size);
void EraseSPIFlashChip(void);
void EraseSPIFlashSector(int SectorNumber) ;


// other prototypes
void Init_RS232(void) ;
int kbhit(void) ;
void Load_SRecordFile(void) ;
void DumpMemory(void) ;
void EnterString(void) ;
void FillMemory(void) ;
void MemoryChange(void) ;
void ProgramFlashChip(void);
void LoadFromFlashChip(void);
void DumpRegisters(void) ;
void DumpRegistersandPause(void) ;
void ChangeRegisters(void);

void Out1Hex(int c) ;
void Out2Hex(int c) ;
void go(void) ;
void main(void) ;
void menu(void) ;
void stop(void) ;
void Help(void) ;
void UnknownCommand(void) ;
void HitAnyKeyToContinue(void) ;
void MemoryTest(void) ;
void FlushKeyboard(void) ;
void TestLEDS(void) ;

// prototypes for disassembler

void DisassembleInstruction(unsigned short int *OpCode);
void Decode6BitEA(unsigned short int *OpCode, int EAChoice, unsigned short int DataSize, unsigned short int IsItMoveInstruction);
void Decode3BitOperandMode(unsigned short int *OpCode) ;  // used with instructions like ADD
void Decode3BitAddressRegister(unsigned short int Reg);
void Decode3BitDataRegister(unsigned short int OpCode) ;
unsigned short int Decode2BitOperandSize(unsigned short int OpCode);
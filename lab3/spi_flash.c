#include <stdio.h>


/*********************************************************************************************
**	RS232 port addresses
*********************************************************************************************/

#define RS232_Control     *(volatile unsigned char *)(0x00400040)
#define RS232_Status      *(volatile unsigned char *)(0x00400040)
#define RS232_TxData      *(volatile unsigned char *)(0x00400042)
#define RS232_RxData      *(volatile unsigned char *)(0x00400042)
#define RS232_Baud        *(volatile unsigned char *)(0x00400044)

/*************************************************************
** Flash Commands
**************************************************************/
#define FLASH_ERASE_SECTOR 0x20
#define FLASH_READ_DATA 0x03
#define FLASH_PAGE_PROGRAM 0x02
#define FLASH_WRITE_ENABLE 0x06
#define FLASH_GET_STATUS_REGISTER1 0x05
#define FLASH_GET_MANUFACTURER_ID 0x90

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

/*********************************************************************************************************
**  Subroutine to provide a low level output function to 6850 ACIA
**  This routine provides the basic functionality to output a single character to the serial Port
**  to allow the board to communicate with HyperTerminal Program
**
**  NOTE you do not call this function directly, instead you call the normal putchar() function
**  which in turn calls _putch() below). Other functions like puts(), printf() call putchar() so will
**  call _putch() also
*********************************************************************************************************/

int _putch( int c)
{
    while((RS232_Status & (char)(0x02)) != (char)(0x02))    // wait for Tx bit in status register or 6850 serial comms chip to be '1'
        ;

    RS232_TxData = (c & (char)(0x7f));                      // write to the data register to output the character (mask off bit 8 to keep it 7 bit ASCII)
    return c ;                                              // putchar() expects the character to be returned
}

/*********************************************************************************************************
**  Subroutine to provide a low level input function to 6850 ACIA
**  This routine provides the basic functionality to input a single character from the serial Port
**  to allow the board to communicate with HyperTerminal Program Keyboard (your PC)
**
**  NOTE you do not call this function directly, instead you call the normal getchar() function
**  which in turn calls _getch() below). Other functions like gets(), scanf() call getchar() so will
**  call _getch() also
*********************************************************************************************************/
int _getch( void )
{
    char c ;
    while((RS232_Status & (char)(0x01)) != (char)(0x01))    // wait for Rx bit in 6850 serial comms chip status register to be '1'
        ;

    return (RS232_RxData & (char)(0x7f));                   // read received character, mask off top bit and return as 7 bit ASCII character
}


/******************************************************************************************
** The following code is for the SPI controller
*******************************************************************************************/
// return true if the SPI has finished transmitting a byte (to say the Flash chip) return false otherwise
// this can be used in a polling algorithm to know when the controller is busy or idle.

int TestForSPITransmitDataComplete(void)    {

    /* TODO replace 0 below with a test for status register SPIF bit and if set, return true */
    int spi_status = SPI_Status;
    return ((spi_status & SPSR_SPIF) == SPSR_SPIF);
}

/************************************************************************************
** initialises the SPI controller chip to set speed, interrupt capability etc.
************************************************************************************/
void SPI_Init(void)
{
    //TODO
    //
    // Program the SPI Control, EXT, CS and Status registers to initialise the SPI controller
    // Don't forget to call this routine from main() before you do anything else with SPI
    //
    // Here are some settings we want to create:
    //
    // Control Reg     - interrupts disabled, core enabled, Master mode, Polarity and Phase of clock = [0,0], speed =  divide by 32 = approx 700Khz
    SPI_Control = !SPCR_SPIE | SPCR_SPE | SPCR_MSTR | !SPCR_CPOL | !SPCR_CPHA | SPCR_SPR;

    // Ext Reg         - in conjunction with control reg, sets speed above and also sets interrupt flag after every completed transfer (each byte)
    SPI_Ext = !SPER_ESPR | !SPER_ICNT;

    // SPI_CS Reg      - control selection of slave SPI chips via their CS# signals
    Disable_SPI_CS();

    // Status Reg      - status of SPI controller chip and used to clear any write collision and interrupt on transmit complete flag
    SPI_Status |= SPSR_SPIF;
    SPI_Status |= SPSR_WCOL;
}

/************************************************************************************
** return ONLY when the SPI controller has finished transmitting a byte
************************************************************************************/
void WaitForSPITransmitComplete(void)
{
    // TODO : poll the status register SPIF bit looking for completion of transmission
    while(!TestForSPITransmitDataComplete());

    // once transmission is complete, clear the write collision and interrupt on transmit complete flags in the status register (read documentation)
    // just in case they were set
    SPI_Status |= SPSR_SPIF;
    SPI_Status |= SPSR_WCOL;
}


/************************************************************************************
** Write a byte to the SPI flash chip via the controller and returns (reads) whatever was
** given back by SPI device at the same time (removes the read byte from the FIFO)
************************************************************************************/
int WriteSPIChar(int c) // int: 32 bits
{
    // todo - write the byte in parameter 'c' to the SPI data register, this will start it transmitting to the flash device
    SPI_Data = (unsigned char) c;

    // wait for completion of transmission
    WaitForSPITransmitComplete();

    // return the received data from Flash chip (which may not be relevent depending upon what we are doing)
    // by reading fom the SPI controller Data Register.
    // note however that in order to get data from an SPI slave device (e.g. flash) chip we have to write a dummy byte to it
    //
    
    // modify '0' below to return back read byte from data register
    //
    
    return SPI_Data;
}

/******************************************************************************************
** The following code is for the flash chip
*******************************************************************************************/

// Get the status of the flash chip before issuing a new command
void flashWaitForIdle(void)
{
    volatile int status = 1;
    Enable_SPI_CS();
    // busy bit is bit 0 of the first status register
    status = WriteSPIChar(FLASH_GET_STATUS_REGISTER1);

    while (status != 0) {
        status = WriteSPIChar(123);
        // printf("Flash chip status register 1: %x\n", status);
        status &= 0x01;
    }  
    Disable_SPI_CS();
}

// Execute the write enable command for the flash chip
void flashWriteEnable(void)
{
    Enable_SPI_CS();
    WriteSPIChar(FLASH_WRITE_ENABLE);
    Disable_SPI_CS();
    flashWaitForIdle();
}

// Expects pageAddress to be 3 bytes
void writeAddressToFlash(unsigned int pageAddress)
{
    WriteSPIChar((pageAddress & 0x00FF0000) >> 16);
    WriteSPIChar((pageAddress & 0x0000FF00) >> 8);
    WriteSPIChar(pageAddress & 0x000000FF);
}

// Erases a sector (4 kbytes: 16 pages) of flash
void flashEraseSector(unsigned int sectorAddress) 
{
    // Poll flash chip for status
    flashWaitForIdle();

    // enable write
    flashWriteEnable();

    // write erase sector command
    Enable_SPI_CS();
    WriteSPIChar(FLASH_ERASE_SECTOR);

    // write address to chip
    writeAddressToFlash(sectorAddress);

    Disable_SPI_CS();

    // Poll flash chip for status
    flashWaitForIdle();
}

// Writes the provided data to a page of flash memory. Length of dataToWrite should be 256 bytes (1 page)
void flashWritePage(unsigned int pageAddress, unsigned char *dataToWrite)
{
    unsigned int i, result, tmp;

    // Poll flash chip for status
    flashWaitForIdle();

    // enable write
    flashWriteEnable();

    // write write page command
    Enable_SPI_CS();
    WriteSPIChar(FLASH_PAGE_PROGRAM);

    // write address to chip
    writeAddressToFlash(pageAddress);

    // write each byte to the SPI controller
    for(i = 0; i < 256; i++) {
        tmp = WriteSPIChar(dataToWrite[i]);
    }

    Disable_SPI_CS();

    // Poll flash chip for status
    flashWaitForIdle();
}

// Read the provided page of flash memory into the provided buffer.  Buffer should be minimum 256 bytes.
// The parameter numBytes should be within range 1-256, inclusive.
void flashRead(unsigned int pageAddress, unsigned char *dataBuf, unsigned int numBytes)
{
    unsigned int i = 0;

    // Poll flash chip for status
    flashWaitForIdle();

    Enable_SPI_CS();

    // write read data command
    WriteSPIChar(FLASH_READ_DATA);

    // write address to chip
    writeAddressToFlash(pageAddress);

    // read each byte by writing garbage data to controller
    for(i = 0; i < numBytes; i++) {
        dataBuf[i] = WriteSPIChar(0xFF);
    }

    Disable_SPI_CS();

    // Poll flash chip for status
    flashWaitForIdle();
}

// Compare 256 bytes of 2 buffers
// If they are not the same, returns the index at which the comparison fails.
// Otherwise, returns -1.
int compareBuffers(unsigned char *buf1, unsigned char *buf2)
{
    int i;
    for(i = 0; i < 256; i++) {
        if (buf1[i] != buf2[i])
            return i;
    }

    return -1;
}

// Prompt user to interact with flash memory
void main(void) 
{
    int i;
    unsigned char functionality = 0, dataSelect = 0;
    unsigned int start_addr, data1 = 0xDEADBEEF, data2 = 0x12345678, flash_start = 0x0, flash_end = 0x???;//TODO

    unsigned char dataBuf[256];
    for (i = 0; i < 256; i++) {
        dataBuf[i] = 0;
    }

    // spi init
    SPI_Init();

    // 3 options: read byte, read sector, write sector
    scanflush();
    printf("\r\nWelcome to the flash memory user program! Please select the functionality to be tested [1 = read a byte, 2 = read a sector, 3 = write a sector]: ");
    scanf("%u", &functionality);
    if (functionality != 1 && functionality != 2 && functionality != 3)
        goto early_exit;

    // Ask for start addr
    printf("\r\nEnter the start address to read or write to (in range [0x%x, 0x%x]): ", flash_start, flash_end);
    scanf("%x", &start_addr);
    if (start addr < || start_addr > ) // TODO
        goto early_exit;

    // perform the read or write
    switch(functionality) {
        case 1: // read a byte
            flashRead(start_addr, dataBuf, 1);
            printf("\r\nByte read at address 0x%x: %x", start_addr, dataBuf[0]);
            break;

        case 2: // read a sector (is this supposed to be a page?)
            flashRead(start_addr, dataBuf, 256);
            printf("\r\nSector read beginning at address 0x%x: ", start_addr);//TODO
            //print data
            break;

        case 3: // write a sector (is this supposed to be a page?)
            printf("\r\nSelect some repeating data to be written to the flash [1 = 0xDEADBEEF, 2 = 0x12345678]: ");
            scanf("%u", &dataSelect);
            flashWritePage(start_addr, dataBuf, dataSelect == 1 ? data1 : data2);
            printf("\r\nData has been written starting at address %x", start_addr);

        default:
            printf("\r\nSomething went very wrong...");
    }

    early_exit: printf("\r\nError");
}
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
void flashReadPage(unsigned int pageAddress, unsigned char *dataBuf)
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
    for(i = 0; i < 256; i++) {
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

// Attempting to write 256kB of data to the flash chip, starting at address 0
// Then, read it back
void main(void) 
{
    int pageNum, sectorNum, result, i;
    unsigned char tmpResult;

    unsigned char dataBuf[256] = {0};

    // Test data for writing flash pages
    unsigned char page1[256] = {0};
    unsigned char page2[256] = {0};
    for (i = 0; i < 256; i++) {
        page1[i] = i;
        page2[i] = 255 - i;
    }

    SPI_Init();
    printf("\nSPI initialization completed!\n");

    // check for manufacturer id to confirm we can talk to the chip
    // flashWaitForIdle();
    // printf("Got past wait for idle\n");
    // Enable_SPI_CS();
    // WriteSPIChar(FLASH_GET_MANUFACTURER_ID);
    // writeAddressToFlash(0);
    // printf("Manufacturer ID: %x (should be 0xEF)\n", WriteSPIChar(0xFF));
    // printf("Device ID: %x (should be 0x15)\n", WriteSPIChar(0xFF));
    // Disable_SPI_CS();

    // Erase 256kB (64 4kB sectors) from the flash, sector by sector (4kB = 2^12 = 4096)
    for(sectorNum = 0; sectorNum < 64; sectorNum++) {
        flashEraseSector(sectorNum * 4096);
    }
    printf("Flash sectors erased.\n");

    // Write 256kB to the flash chip in 256byte chunks
    for(pageNum = 0; pageNum < 1000; pageNum++) { // was 1000
        flashWritePage(pageNum * 256, (pageNum % 2 == 0 ? page1 : page2));
    }
    printf("Flash chip written.\n");

    // Read back the entire program page by page, comparing to the data originally written to ensure correctness
    for(pageNum = 0; pageNum < 1000; pageNum++) { // was 1000
        flashReadPage(pageNum * 256, dataBuf);

        // compare to the page originally written
        result = compareBuffers(dataBuf, (pageNum % 2 == 0 ? page1 : page2));
        if (result != -1)
            printf("Compare failed on page %d at byte %d: Expected: %d Read: %d\n", pageNum, result, (pageNum % 2 == 0 ? page1[result] : page2[result]), dataBuf[result]);
    }

    printf("Flash Memory Test completed!\n");

    while(1); // stop!!!
}
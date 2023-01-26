#include <stdio.h>
#include <string.h>
#include <ctype.h>

/*********************************************************************************************
**	RS232 port addresses
*********************************************************************************************/

#define RS232_Control     *(volatile unsigned char *)(0x00400040)
#define RS232_Status      *(volatile unsigned char *)(0x00400040)
#define RS232_TxData      *(volatile unsigned char *)(0x00400042)
#define RS232_RxData      *(volatile unsigned char *)(0x00400042)
#define RS232_Baud        *(volatile unsigned char *)(0x00400044)


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

// converts hex char to 4 bit binary equiv in range 0000-1111 (0-F)
// char assumed to be a valid hex char 0-9, a-f, A-F

char xtod(int c)
{
    if ((char)(c) <= (char)('9'))
        return c - (char)(0x30);    // 0 - 9 = 0x30 - 0x39 so convert to number by sutracting 0x30
    else if((char)(c) > (char)('F'))    // assume lower case
        return c - (char)(0x57);    // a-f = 0x61-66 so needs to be converted to 0x0A - 0x0F so subtract 0x57
    else
        return c - (char)(0x37);    // A-F = 0x41-46 so needs to be converted to 0x0A - 0x0F so subtract 0x37
}

// Returns 0 if addresses are valid, 1 otherwise
int get_address_range(unsigned int *start_addr, unsigned int *end_addr, unsigned int width_option)
{
    scanflush();
    printf("\nEnter a start address for the test in hex (if word or long word was selected, ensure this address is even): ");
    scanf("%x", start_addr);

    scanflush();
    printf("\nEnter an end address for the test in hex (if word or long word was selected, ensure this address is even): ");
    scanf("%x", end_addr);
    printf("\nStart: %x, End: %x", *start_addr, *end_addr);

    // word/longword
    if (width_option != 1 && (((*start_addr) % 2 != 0) || ((*end_addr) % 2 != 0))) 
    {
        printf("\nInvalid start or end address: must be even");
        return 1;
    }

    // check addr range 0802 0000 –----- 0803 0000
    if (*start_addr < 0x08020000 || (*start_addr + width_option) > 0x08030000 || *end_addr < 0x08020000 || (*end_addr + width_option) > 0x08030000)
    {
        printf("\nInvalid start or end address: must be in 0x0802 0000 to 0x0803 0000 range");
        return 1;
    }

    // start addr < end addr
    if (*start_addr >= *end_addr)
    {
        printf("\nInvalid addresses: start address must be before or equal to end address");
        return 1;
    }

    return 0;
}

int get_data_pattern(unsigned int *pattern, unsigned int width_option)
{
    unsigned char pattern_input[9] = "~~~~~~~~";
    unsigned int i = 0, j = 0, k = 0;

    for (i = 0; i < 4; i++) 
    {
        scanflush();
        printf("\nPlease enter hexadecimal string %d of 4, of length %u characters: ", i+1, width_option*2);
        scanf("%8s", pattern_input);
        printf("\nPattern input: %s\n", pattern_input);
        
        j = 0;
        pattern[i] = xtod(pattern_input[j]);
        for (j = 1; j < 2*width_option; j++) 
        {
            printf("\ninput %d: \'%x\'\n", j, pattern_input[j]);
            if (pattern_input[j] == '~')
            {
                printf("\nHexadecimal string is not long enough");
                return 1;
            }
            pattern[i] = (pattern[i] << 4) | xtod(pattern_input[j]); 
        }
        
        printf("\npattern %u is %x", i+1, pattern[i]);
        for (k = 0; k < 8; k++)
        {
            pattern_input[k] = '~';
        }
    }
    
    return 0;
}

void main()
{  
    unsigned int pattern[4] = {0,0,0,0};
    unsigned int width_option = 0, start_addr = 0, end_addr = 0, i = 0, j = 0, value_from_mem = 0;

    // GET USER INPUTS

    scanflush();
    printf("\r\nSelect value width for test [1 = bytes, 2 = words, 4 = long words]: ");
    scanf("%u", &width_option);
    if (width_option != 1 && width_option != 2 && width_option != 4)
    {
        printf("\nInvalid value width selected");
        goto early_exit;
    }

    if (get_data_pattern(pattern, width_option) != 0 || get_address_range(&start_addr, &end_addr, width_option) != 0)
    {
        goto early_exit;
    }

    // MEMORY TEST
    for (i = start_addr; i <= end_addr; i += width_option)
    {
        switch(width_option) 
        {
            case 1:
                *((char *)(i)) = pattern[j];
                value_from_mem = *((char *)(i));
                if ((unsigned char)value_from_mem != pattern[j]) {
                    printf("\nFAIL at address %x: value written to memory was %x, value read from memory was %x", i, pattern[j], (unsigned char) value_from_mem);
                    goto early_exit;
                }
                break;
            case 2:
                *((unsigned short int *)(i)) = pattern[j];
                value_from_mem = *((unsigned short int *)(i));
                if ((unsigned short int)value_from_mem != pattern[j]) {
                    printf("\nFAIL at address %x: value written to memory was %x, value read from memory was %x", i, pattern[j], (unsigned short int) value_from_mem);
                    goto early_exit;
                }
                break;
            case 4:
                *((unsigned int *)(i)) = pattern[j];
                value_from_mem = *((unsigned int *)(i));
                if ((unsigned int)value_from_mem != pattern[j]) {
                    printf("\nFAIL at address %x: value written to memory was %x, value read from memory was %x", i, pattern[j], (unsigned int) value_from_mem);
                    goto early_exit;
                }
                break;
            default:
                printf("\r\nSomething went very wrong...");
                goto early_exit;
        }

        // print progress
        if ((i - start_addr) % 10001 == 0)
        {
            printf("\nAddress: %x Data read/written: %x\n", i, (unsigned int) value_from_mem);
        }

        j = (j + 1) % 4;
    }

    early_exit: printf("\nFAIL");

}

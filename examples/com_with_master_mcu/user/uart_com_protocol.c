#include "uart_com_protocol.h"

u8 UartReciBuff[20];  
u8 UartReciBuffLen;  
bool IsDataReady = false;  
void protocol_input(u8 data)  
{  
    static int state = 0;  
    static int count = 0;  
    if(state == 0)  
    {  
        if(IsDataReady == false)  
        {  
            if(data == 'T')  
            {  
                state = 1;  
                count = 0;  
            }  
        }  
    }  
    else if(state == 1)  
    {  
        if(data == '\\')  
        {  
            state = 2;  
        }  
        else if(data == 'Q')  
        {  
            state = 0;  
            IsDataReady = true;  
            UartReciBuffLen = count;  
            printf("data ready:%d\r\n",count);  
        }  
        else  
        {  
            UartReciBuff[count++] = data;  
        }  
    }  
    else if(state == 2)  
    {  
        state = 1;  
        UartReciBuff[count++] = data;  
    }  
}  
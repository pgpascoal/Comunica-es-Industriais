#include <stdio.h>
#include <string.h>	//strlen
#include <sys/socket.h>
#include <arpa/inet.h>	//inet_addr
#include <unistd.h>	//write

#include <stdlib.h>

#include "ModbusAP.h"
#include "ModbusTCP.h"

#define max_number_of_registers 65536
#define max_quantity_of_registers 123
#define ADPU_max_size 252 // 123*2 + 6
#define Write_multiple_request_function_code 0x10
#define Read_holding_registers_function_code 0x03

#define STATE_START 0
#define STATE_ERROR -1
#define STATE_RESPONSE 1
#define STATE_EXCEPTION 4
#define STATE_OK 2


void print_buffer(char *buffer, int size){
    printf("Buffer: ");
    for(int i = 0; i < size; i++) printf("%02X ", buffer[i]);
    printf("\n");
}

int Read_h_regs(int server_add, int port, int st_r, int n_r, char *val){
    // Check Parameters
    if((st_r > max_number_of_registers) || (st_r < 0)){
        printf("[-] Bad parameters (st_r)\n");
        return -1;
    }
    if((n_r > max_number_of_registers) || (n_r < 0)){
        printf("[-] Bad parameters (n_r)\n");
        return -1;
    }
    if(n_r + st_r > max_number_of_registers){
        printf("[-] Bad parameters (st_r + n_r)\n");
        return -1;
    }
    if(!val){
        //printf("[-] Bad parameters (val)\n");
        //return -1;
    }

    // Assembles APDU (MODBUS PDU)
    char ADPU[ADPU_max_size];
    uint8_t Func_code, HI_byte_st, LO_byte_st, HI_byte_quantity, LO_byte_quantity; // 5 bytes overhead

    Func_code = Read_holding_registers_function_code;
    HI_byte_st = (st_r - 1) >> 8;
    LO_byte_st = (st_r - 1) & 0xFF;
    HI_byte_quantity = n_r >> 8;
    LO_byte_quantity = n_r & 0xFF;

    ADPU[0] = Func_code;
    ADPU[1] = HI_byte_st;
    ADPU[2] = LO_byte_st;
    ADPU[3] = HI_byte_quantity;
    ADPU[4] = LO_byte_quantity;

    int ADPU_size = (5);
    char *ADPU_R;

ADPU_R[0] = 'a';
    // Send_Modbus_request()
    int res = Send_Modbus_request(server_add, port, ADPU, ADPU_size, ADPU_R);

    // checks the reponse (APDU_R or error_code)
    if(res < 0){
        printf("[-] Send_Modbus_request() call failed\n");
        return res;
    }

    int STATE = 0, Exception_code, ADPU_R_byte_count, read_registers = 0;

    while(STATE != STATE_ERROR){
        switch (STATE)
        {
        case (STATE_START):
            if(ADPU_R[0] == 0x03) STATE = STATE_RESPONSE;
            if(ADPU_R[0] == 0x83) STATE = STATE_EXCEPTION;
            else STATE = STATE_ERROR;
            break;
        case (STATE_RESPONSE):
            ADPU_R_byte_count = ADPU_R[1];
            for(int i = 0; i < ADPU_R_byte_count; i++){
                val[i] = ADPU_R[i+2];
                read_registers++;
            }
            return read_registers;
            break;
        case (STATE_EXCEPTION):
            Exception_code = ADPU_R[1];
            return Exception_code;
            break;
        default:
            STATE = STATE_ERROR;
            break;
        }
    }
    // returns: number of read registers - ok, <0 - Error

    return read_registers;
}

int Write_multiple_request(int server_add, int port, int st_r, int n_r, char *val){
    // Check Parameters
    // port
    if((st_r > max_number_of_registers) || (st_r < 0)){
        printf("[-] Bad parameters (st_r)\n");
        return -1;
    }
    if((n_r > max_number_of_registers) || (n_r < 0)){
        printf("[-] Bad parameters (n_r)\n");
        return -1;
    }
    if(n_r + st_r > max_number_of_registers){
        printf("[-] Bad parameters (st_r + n_r)\n");
        return -1;
    }
    if(!val){
        //printf("[-] Bad parameters (val)\n");
        //return -1;
    }

    // Assembles ADPU (Modbus PDU)
    char ADPU[ADPU_max_size];
    uint8_t Func_code, HI_byte_st, LO_byte_st, HI_byte_quantity, LO_byte_quantity, Byte_count, HI_reg, LO_Reg;

    Func_code = Write_multiple_request_function_code;
    HI_byte_st = (st_r - 1) >> 8;
    LO_byte_st = (st_r - 1) & 0xFF;
    HI_byte_quantity = n_r >> 8;
    LO_byte_quantity = n_r & 0xFF;
    Byte_count = 2 * n_r;


    ADPU[0] = Func_code;
    ADPU[1] = HI_byte_st;
    ADPU[2] = LO_byte_st;
    ADPU[3] = HI_byte_quantity;
    ADPU[4] = LO_byte_quantity;
    ADPU[5] = Byte_count;

    for(int i = 0; i < 2*n_r; i++){
        //HI_reg == val[i] >> 8;
        //LO_Reg == val[i] && 0xFF;
        //ADPU[2*i + 6] = 0x00;
        //ADPU[2*i+1 + 6] = val[i];
        ADPU[i + 6] = val[i];
    }
        
    int ADPU_size = (6 + 2*n_r);
    char *ADPU_R;

    
    print_buffer(val, (2*n_r));
    print_buffer(ADPU, (6 + 2*n_r));

    // Send_Modbus_request()
    int res = Send_Modbus_request(server_add, port, ADPU, ADPU_size, ADPU_R);

    // Checks the response (APDU_R or error_code)
    if(res < 0){
        printf("[-] Send_Modbus_request() call failed\n");
        return res;
    }

    int STATE = STATE_START, Exception_code, ADPU_R_byte_count, write_registers = 0, HI_starting_address, LO_starting_address, HI_quantity_of_registers, LO_quantity_of_registers;
    return 1;
    while(STATE != STATE_ERROR){
        switch (STATE)
        {
        case (STATE_START):
            if(ADPU_R[0] == 0x10) STATE = STATE_RESPONSE;
            if(ADPU_R[0] == 0x90) STATE = STATE_EXCEPTION; 
            else STATE = STATE_ERROR;
            break;
        case (STATE_RESPONSE):
            HI_starting_address = ADPU_R[1];
            LO_starting_address = ADPU_R[2];
            HI_quantity_of_registers = ADPU_R[3];
            LO_quantity_of_registers = ADPU_R[4];

            write_registers = (HI_quantity_of_registers << 8) || LO_quantity_of_registers;
            return write_registers;
            break;
        case (STATE_EXCEPTION):
            Exception_code = ADPU_R[1];
            break;
        default:
            STATE = STATE_ERROR;
            break;
        }
    }

    // Return: number of written register - ok ; <0 - Error
    return 1;
}
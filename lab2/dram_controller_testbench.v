module dram_controller_testbench();
    reg Clock, Reset_L, UDS_L, LDS_L, WE_L, AS_L, DramSelect_L;
    reg [31:0] Address;
    reg [15:0] DataIn;
     
    wire Dtack_L, ResetOut_L;
    wire SDram_CKE_H, SDram_CS_L, SDram_RAS_L, SDram_CAS_L, SDram_WE_L;
    wire [15:0] DataOut;
    wire [12:0] SDram_Addr;
    wire [1:0] SDram_BA;
    wire [15:0] SDram_DQ;
    wire [4:0] DramState;

    M68kDramController_Verilog dut(
            Clock, Reset_L, Address, DataIn, UDS_L, LDS_L,
			DramSelect_L, WE_L, AS_L, DataOut, SDram_CKE_H,
			SDram_CS_L, SDram_RAS_L, SDram_CAS_L, SDram_WE_L,
			SDram_Addr,	SDram_BA, SDram_DQ, Dtack_L, ResetOut_L,
	        DramState
	); 	

    reg [2:0] refreshtimerloadcount;

    // clock 
    initial begin
        Clock = 0;
        forever begin
            #5;
            Clock = ~Clock;
        end
    end

    initial begin
        // reset
        Reset_L = 0;
        #10;
        Reset_L = 1;
        
        // sit back and watch?
        refreshtimerloadcount = 0;
        while (refreshtimerloadcount < 3'd2) begin
            #10;
            if (DramState == 5'h0F) begin // LoadRefreshTimer
                refreshtimerloadcount = refreshtimerloadcount + 1;
            end
        end

        $display("Simulation should be over!");
        #5;
        $stop;
    end
    
endmodule
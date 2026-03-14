`timescale 1ns / 1ps
module TestBench();	
parameter   t   = 20;
parameter   th   = t*0.5;

reg clk;
always #th clk = ~clk;

reg data_in, op_st, rst;
reg [1:0] mode;

reg [31:0] addr_input = 0, addr_output = 0;
reg [31:0] din_input, din_output;
wire [31:0] dout_input, dout_output;
reg en_input, we_input, en_output, we_output;
wire done;

myip_v1_0 myip(
	.addr_input(addr_input),
	.clk_input(clk),
	.din_input(din_input),
	.dout_input(dout_input),
	.en_input(en_input),
	.we_input(we_input),
		
	.addr_output(addr_output),
	.clk_output(clk),
	.din_output(din_output),
	.dout_output(dout_output),
	.en_output(en_output),
	.we_output(we_output),
	
	.s00_axi_aclk(clk),
	.data_in(data_in),
	.op_st(op_st),
	.rst(rst),
	.mode(mode),
	.done(done)
);

wire [10:0] address;
assign address = ( mode == 2'b00 ) ? 513 : 1025;
reg [31:0] temp_address = 0;
always @( posedge clk) begin 
	if ( done ) begin 
		if ( addr_output < address*4) begin 
			temp_address <= temp_address + 4;
			addr_output <= temp_address;
			if (addr_output != 0) begin 
				if (dout_output == 0) $display("0.0");
				else $display("%.10g", $signed(dout_output) /64.0 );
			end
		end
	end
end

//open file============================================================================
// open input data file
integer input_file_1;
initial begin 
	input_file_1 = $fopen("D:/research/comparison/conv1Input_integer.txt", "r");
	if (input_file_1 == 0) begin
		$display("Input file 1 open failed!");
		$finish;
	end
end

integer input_file_2;
initial begin 
	input_file_2 = $fopen("D:/research/comparison/conv2Input_integer.txt", "r");
	if (input_file_2 == 0) begin
		$display("Input file 2 open failed!");
		$finish;
	end
end

integer input_file_3;
initial begin 
	input_file_3 = $fopen("D:/research/comparison/conv3Input_integer.txt", "r");
	if (input_file_3 == 0) begin
		$display("Input file 3 open failed!");
		$finish;
	end
end

integer input_file_4;
initial begin 
	input_file_4 = $fopen("D:/research/comparison/conv4Input_integer.txt", "r");
	if (input_file_4 == 0) begin
		$display("Input file 4 open failed!");
		$finish;
	end
end

// open  conv bias file
integer bias_file_1;
initial begin 
	bias_file_1 = $fopen("D:/research/comparison/conv1data/conv1B_integer_copy.txt", "r");
	if (bias_file_1 == 0) begin
		$display("Bias file 1 open failed!");
		$finish;
	end
end

integer bias_file_2;
initial begin 
	bias_file_2 = $fopen("D:/research/comparison/conv2data/conv2B_integer_copy.txt", "r");
	if (bias_file_2 == 0) begin
		$display("Bias file 2 open failed!");
		$finish;
	end
end

integer bias_file_3;
initial begin 
	bias_file_3 = $fopen("D:/research/comparison/conv3data/conv3B_integer_copy.txt", "r");
	if (bias_file_3 == 0) begin
		$display("Bias file 3 open failed!");
		$finish;
	end
end

integer bias_file_4;
initial begin 
	bias_file_4 = $fopen("D:/research/comparison/conv4data/conv4B_integer_copy.txt", "r");
	if (bias_file_4 == 0) begin
		$display("Bias file 4 open failed!");
		$finish;
	end
end
// read input & bias data ============================================================================
integer bias_file_status;
integer bias_val;
reg [31:0] temp_addr_bias = 0;

integer input_file_status;
integer input_val;
reg [31:0] temp_addr_a = 0;

integer i;
initial begin 	
	clk = 0;
	data_in = 0; op_st = 0; rst = 0;
	#th
	/*
	// layer 1 ===========================================================================
	rst = 1;
	#t
	
	rst = 0;
	#t 
	
	data_in = 1;
	// read conv1 input data
	en_input = 1; we_input = 1;	
	for ( i = 0 ; i < 256 ; i = i + 1) begin 
		if (!$feof(input_file_1)) begin
			input_file_status = $fscanf(input_file_1, "%f\n", input_val);
			din_input[5:0] = input_val;
			temp_addr_a <= temp_addr_a + 4;
			addr_input <= temp_addr_a;
		end
		#t;
	end
	temp_addr_a = 0;
	
	// read conv1 bias data
	en_output = 1; we_output = 1;
	for ( i = 0 ; i < 1024; i = i + 1) begin 
		if ( !$feof(bias_file_1) ) begin
			bias_file_status = $fscanf(bias_file_1, "%f\n", bias_val);
			din_output = bias_val;
			temp_addr_bias <= temp_addr_bias + 4;
			addr_output <= temp_addr_bias;
		end
		#t;
	end
	temp_addr_bias = 0;
	addr_output = 0;
	
	data_in = 0;
	#t
	
	op_st = 1;
	en_input = 0; we_input = 0;
	en_output = 0; we_output = 0;
	mode = 2'b00;
	#(t*2128) //42560ns
	
	data_in = 0; op_st = 0; 
	#( t*(512 + 3 ) ) // 
	

	// layer 2 ===========================================================================
	data_in = 0; op_st = 0; rst = 1;
	#t 
	
	rst = 0;
	#t
	
	data_in = 1;
	// read conv2 input data
	en_input = 1; we_input = 1;	
	for ( i = 0 ; i < 512 ; i = i + 1) begin 
		if (!$feof(input_file_2)) begin
			input_file_status = $fscanf(input_file_2, "%f\n", input_val);
			din_input[5:0] = input_val;
			temp_addr_a <= temp_addr_a + 4;
			addr_input <= temp_addr_a;
		end
		#t;
	end
	temp_addr_a = 0;
	
	// read conv2 bias data
	en_output = 1; we_output = 1;
	for ( i = 0 ; i < 2048; i = i + 1) begin 
		if ( !$feof(bias_file_2) ) begin
			bias_file_status = $fscanf(bias_file_2, "%f\n", bias_val);
			din_output = bias_val;
			temp_addr_bias <= temp_addr_bias + 4;
			addr_output <= temp_addr_bias;
		end
		#t;
	end
	temp_addr_bias = 0;
	addr_output = 0;
	
	data_in = 0;
	#t
	
	op_st = 1;
	en_input = 0; we_input = 0;
	en_output = 0; we_output = 0;
	mode = 2'b01;
	#(t*11460) //229200ns
	
	data_in = 0; op_st = 0; 
	//#( t*(1024 + 3 ) ) // 


	// layer 3 ===========================================================================
	data_in = 0; op_st = 0; rst = 1;
	#t 
	
	rst = 0;
	#t
	
	data_in = 1;
	// read conv3 input data
	en_input = 1; we_input = 1;	
	for ( i = 0 ; i < 1024 ; i = i + 1) begin 
		if (!$feof(input_file_3)) begin
			input_file_status = $fscanf(input_file_3, "%f\n", input_val);
			din_input[5:0] = input_val;
			temp_addr_a <= temp_addr_a + 4;
			addr_input <= temp_addr_a;
		end
		#t;
	end
	temp_addr_a = 0;
	
	// read conv3 bias data
	en_output = 1; we_output = 1;
	for ( i = 0 ; i < 2048; i = i + 1) begin 
		if ( !$feof(bias_file_3) ) begin
			bias_file_status = $fscanf(bias_file_3, "%f\n", bias_val);
			din_output = bias_val;
			temp_addr_bias <= temp_addr_bias + 4;
			addr_output <= temp_addr_bias;
		end
		#t;
	end
	temp_addr_bias = 0;
	addr_output = 0;
	
	data_in = 0;
	#t
	
	op_st = 1;
	en_input = 0; we_input = 0;
	en_output = 0; we_output = 0;
	mode = 2'b10;
	#(t*44548) //890960ns
	
	data_in = 0; op_st = 0; 
	//#( t*(1024 + 3 ) ) // 
	
	*/
	
	// layer 4 ===========================================================================
	data_in = 0; op_st = 0; rst = 1;
	#t 
	
	rst = 0;
	#t
	
	data_in = 1;
	// read conv4 input data
	en_input = 1; we_input = 1;	
	for ( i = 0 ; i < 1024 ; i = i + 1) begin 
		if (!$feof(input_file_4)) begin
			input_file_status = $fscanf(input_file_4, "%f\n", input_val);
			din_input[5:0] = input_val;
			temp_addr_a <= temp_addr_a + 4;
			addr_input <= temp_addr_a;
		end
		#t;
	end
	temp_addr_a = 0;
	
	// read conv4 bias data
	en_output = 1; we_output = 1;
	for ( i = 0 ; i < 2048; i = i + 1) begin 
		if ( !$feof(bias_file_4) ) begin
			bias_file_status = $fscanf(bias_file_4, "%f\n", bias_val);
			din_output = bias_val;
			temp_addr_bias <= temp_addr_bias + 4;
			addr_output <= temp_addr_bias;
		end
		#t;
	end
	temp_addr_bias = 0;
	addr_output = 0;
	
	data_in = 0;
	#t
	
	op_st = 1;
	en_input = 0; we_input = 0;
	en_output = 0; we_output = 0;
	mode = 2'b11;
	#(t*106500) //2130000ns
	
	data_in = 0; op_st = 0; 
	#( t*(1024 + 3 ) ) // 


	$fclose(bias_file_1);	$fclose(input_file_1);
	$fclose(bias_file_2);	$fclose(input_file_2);
	$fclose(bias_file_3);	$fclose(input_file_3);
	$fclose(bias_file_4);	$fclose(input_file_4);
	
	$finish;
end


endmodule

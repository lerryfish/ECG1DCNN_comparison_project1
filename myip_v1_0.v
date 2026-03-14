
`timescale 1 ns / 1 ps

module myip_v1_0 #
(
	// Users to add parameters here

	// User parameters ends
	// Do not modify the parameters beyond this line


	// Parameters of Axi Slave Bus Interface S00_AXI
	parameter integer C_S00_AXI_DATA_WIDTH	= 32,
	parameter integer C_S00_AXI_ADDR_WIDTH	= 4
)
(
	// Users to add ports here

	//bram0	// control input data 
	input wire [12:0] addr_input,
	input wire clk_input,
	input wire [31:0] din_input,
	output wire [31:0] dout_input,
	input wire en_input,
	input wire we_input,
		
	//bram2	// control bias and output
	input wire [12:0] addr_output,
	input wire clk_output,
	input wire [31:0] din_output,
	output wire [31:0] dout_output,
	input wire en_output,
	input wire we_output,
	

	// User ports ends
	// Do not modify the ports beyond this line


	// Ports of Axi Slave Bus Interface S00_AXI
	input wire  s00_axi_aclk,
	input wire  s00_axi_aresetn,
	input wire [C_S00_AXI_ADDR_WIDTH-1 : 0] s00_axi_awaddr,
	input wire [2 : 0] s00_axi_awprot,
	input wire  s00_axi_awvalid,
	output wire  s00_axi_awready,
	input wire [C_S00_AXI_DATA_WIDTH-1 : 0] s00_axi_wdata,
	input wire [(C_S00_AXI_DATA_WIDTH/8)-1 : 0] s00_axi_wstrb,
	input wire  s00_axi_wvalid,
	output wire  s00_axi_wready,
	output wire [1 : 0] s00_axi_bresp,
	output wire  s00_axi_bvalid,
	input wire  s00_axi_bready,
	input wire [C_S00_AXI_ADDR_WIDTH-1 : 0] s00_axi_araddr,
	input wire [2 : 0] s00_axi_arprot,
	input wire  s00_axi_arvalid,
	output wire  s00_axi_arready,
	output wire [C_S00_AXI_DATA_WIDTH-1 : 0] s00_axi_rdata,
	output wire [1 : 0] s00_axi_rresp,
	output wire  s00_axi_rvalid,
	input wire  s00_axi_rready
);
// Instantiation of Axi Bus Interface S00_AXI
wire data_in, op_st, rst, done;
wire [1:0] mode;

myip_v1_0_S00_AXI # ( 
	.C_S_AXI_DATA_WIDTH(C_S00_AXI_DATA_WIDTH),
	.C_S_AXI_ADDR_WIDTH(C_S00_AXI_ADDR_WIDTH)
) myip_v1_0_S00_AXI_inst (

	.op_st(op_st),
	.data_in(data_in),
	.done(done),
	.rst(rst),
	.mode(mode),

	.S_AXI_ACLK(s00_axi_aclk),
	.S_AXI_ARESETN(s00_axi_aresetn),
	.S_AXI_AWADDR(s00_axi_awaddr),
	.S_AXI_AWPROT(s00_axi_awprot),
	.S_AXI_AWVALID(s00_axi_awvalid),
	.S_AXI_AWREADY(s00_axi_awready),
	.S_AXI_WDATA(s00_axi_wdata),
	.S_AXI_WSTRB(s00_axi_wstrb),
	.S_AXI_WVALID(s00_axi_wvalid),
	.S_AXI_WREADY(s00_axi_wready),
	.S_AXI_BRESP(s00_axi_bresp),
	.S_AXI_BVALID(s00_axi_bvalid),
	.S_AXI_BREADY(s00_axi_bready),
	.S_AXI_ARADDR(s00_axi_araddr),
	.S_AXI_ARPROT(s00_axi_arprot),
	.S_AXI_ARVALID(s00_axi_arvalid),
	.S_AXI_ARREADY(s00_axi_arready),
	.S_AXI_RDATA(s00_axi_rdata),
	.S_AXI_RRESP(s00_axi_rresp),
	.S_AXI_RVALID(s00_axi_rvalid),
	.S_AXI_RREADY(s00_axi_rready)
);

// Add user logic here

reg conv_and_relu_done;
always @(posedge s00_axi_aclk) begin 
	if (rst) conv_and_relu_done <= 0;
	else if ( cnt_w[weight_check_bit] == 1'b1 )	conv_and_relu_done <= 1;
	else conv_and_relu_done <= conv_and_relu_done;
end

// Add user logic here
wire [2:0] input_channel_check_bit;
wire [5:0] output_channel_check_bit;
wire [3:0] in_vector_size;
wire [3:0] in_vector_size_check_bit;
wire [3:0] weight_check_bit;
wire [11:0]pooling_max_check_bit;

assign input_channel_check_bit = (mode ==2'b00) ? 0 : (mode == 2'b01) ? 2 : (mode == 2'b10) ? 4 : 5 ;
assign output_channel_check_bit = (mode ==2'b00) ? 2 : (mode == 2'b01) ? 4 : (mode == 2'b10) ? 5 : 6 ;
//assign in_vector_size = (mode ==2'b00) ? 256 : (mode == 2'b01) ? 128 : (mode == 2'b10) ? 64 : 32 ;
assign in_vector_size = (mode ==2'b00) ? 8 : (mode == 2'b01) ? 7 : (mode == 2'b10) ? 6 : 5 ;
assign in_vector_size_check_bit = (mode ==2'b00) ? 8 : (mode == 2'b01) ? 7 : (mode == 2'b10) ? 6 : 5 ;
assign weight_check_bit = (mode == 2'b00) ? 2 : (mode == 2'b01) ? 6 : (mode == 2'b10) ? 9 : (mode == 2'b11) ? 11: 0;
assign pooling_max_check_bit = (mode == 2'b00) ? 10 : 11;

//assign input_channel_number = (mode ==2'b00) ? 0 : (mode == 2'b01) ? 3 : (mode == 2'b10) ? 15 : (mode == 2'b11) ? 31 : 5'bzzzzz;
//assign output_channel_number = (mode ==2'b00) ? 3 : (mode == 2'b01) ? 15 : (mode == 2'b10) ? 31 : (mode == 2'b11) ? 63 : 6'bzzzzzz;
//assign in_vector_size = (mode ==2'b00) ? 255 : (mode == 2'b01) ? 127 : (mode == 2'b10) ? 63 : (mode == 2'b11) ? 31 : 10'dz;
//assign weight_num = (mode == 2'b00) ? 3 : (mode == 2'b01) ? 63 : (mode == 2'b10) ? 511 : (mode == 2'b11) ? 2047: 11'dz;
//assign pooling_max_address = (mode == 2'b00) ? 1023 : 2047;

// input data bram signal==============================================================================================
reg op_st_delay ;
always @(posedge s00_axi_aclk) begin 
	op_st_delay <= op_st;
end
	
wire bram_input_ena, bram_input_wea;
wire [9:0] bram_input_addra;
wire [5:0] bram_input_dina, bram_input_douta;
reg [10:0] cnt_a;

assign bram_input_ena = data_in ? en_input : ( op_st_delay ) ? 1 : 0;
assign bram_input_wea = data_in ? we_input : 0;
assign bram_input_addra = data_in ? addr_input >> 2 : op_st_delay ? cal_addra : 0 ;
assign bram_input_dina = data_in ? din_input[5:0] : 0;

wire [9:0] cal_addra;
assign cal_addra = ( done == 0 ) ? cnt_a : 0 ;

// cnt_a is brama address, when calculate
reg [6:0] input_repeat_time;	// count output channel
reg [5:0] acc_times;	// count accumulate times, count input channel
reg [2:0] for_cnt_a;
always@(posedge s00_axi_aclk) begin
	if(rst) begin 
		input_repeat_time <= 1;
		acc_times <= 1;
		cnt_a <= 0 ;
		for_cnt_a <= 0;
	end
	else if ( kernel_end_out ) begin 
		if ( acc_times[input_channel_check_bit] == 1'b1 ) begin 
			cnt_a <= 0;
			acc_times <= 1 ;
			if ( input_repeat_time[output_channel_check_bit] == 1'b0 ) input_repeat_time <= input_repeat_time + 1;
		end
		else begin 
			cnt_a <= cnt_a + 1;
			acc_times <= acc_times + 1;
		end
		for_cnt_a <= 0;
	end		
	
	else  if ( cnt_a == ( (acc_times << in_vector_size) -1 ) ) begin 
		//cnt_a == ( (in_vector_size) * (acc_times) - 1 ) 
		cnt_a <= cnt_a;
	end
	
	else if(op_st_delay)  begin 
		if ( for_cnt_a == 6 ) cnt_a <= cnt_a + 1;
		else for_cnt_a <= for_cnt_a + 1;
	end
	else cnt_a <= cnt_a ;
end

wire bram_input_enb, bram_input_web;
wire [9:0] bram_input_addrb;
wire [5:0] bram_input_dinb, bram_input_doutb;
blk_mem_gen_a brama(
	.clka(s00_axi_aclk), 
	.addra(bram_input_addra), 
	.dina(bram_input_dina), 
	.douta(bram_input_douta), 
	.wea(bram_input_wea), 
	.ena(bram_input_ena),
	
	
	.clkb(s00_axi_aclk),
	.addrb(bram_input_addrb),
	.dinb(bram_input_dinb),
	.doutb(bram_input_doutb),
	.web(bram_input_web),
	.enb(bram_input_enb)
) ; 

// weight bram signal==================================================================================================
wire bram_enw, bram_wew;
wire [11:0] bram_addrw;
wire [63:0] bram_dinw, bram_doutw;
reg [11:0] cnt_w;
assign bram_enw = ( op_st && conv_and_relu_done == 0) ? 1 : 0;
assign bram_wew = 0;
assign bram_addrw = (op_st) ? cnt_w+base_address_weight : 0 ;
assign bram_dinw = 0;

wire [9:0] base_address_weight;
assign base_address_weight = (mode == 2'b00) ? 0 : 
							(mode == 2'b01) ? 4 :
							(mode == 2'b10) ? 68 : 580 ;

always@(posedge s00_axi_aclk) begin
	if(rst) cnt_w <= 0 ;
	else  if ( cnt_w[weight_check_bit] == 1'b1 ) cnt_w <= cnt_w;
	else if( kernel_end_out == 1 ) begin 
		cnt_w <= cnt_w + 1;
	end
	else cnt_w <= cnt_w ;
end

blk_mem_gen_accw bramw(
	.clka(s00_axi_aclk), 
	.addra(bram_addrw), 
	.dina(bram_dinw), 
	.douta(bram_doutw), 
	.wea(bram_wew), 
	.ena(bram_enw)
	
) ; 


// bias & output bram signal==========================================================================================
// bias signal
wire bram_en_bias, bram_we_bias;
wire [10:0] bram_addr_bias;
wire [20:0] bram_din_bias;
assign bram_en_bias = data_in ? en_output : 0;
assign bram_we_bias = data_in ? we_output : 0;
assign bram_addr_bias = data_in ? addr_output >> 2 : 0 ;
assign bram_din_bias = data_in ? din_output : 0;

// port A // store bias // when operate read port 
wire bram_ena_output, bram_wea_output;
wire [10:0] bram_addra_output;
wire [20:0] bram_dina_output, bram_douta_output;
assign bram_ena_output = data_in ? bram_en_bias : (kernel_start_out) ? 1 : ( pooling_en ) ? 1 : (done) ? 1 : 0;
assign bram_wea_output = data_in ? bram_we_bias : 0;
assign bram_addra_output = data_in ? bram_addr_bias : 
						(kernel_start_out) ? (base_address+kernel_output_num-1) : 
						( pooling_en ) ? pooling_addr_read[10:0] :
						(done) ? addr_output >> 2 : 0  ;
assign bram_dina_output = data_in ? bram_din_bias  : 0;

wire [10:0] base_address;
assign base_address = (input_repeat_time-1) << (in_vector_size);

// port B	// when operate write port
reg [10:0] bram_addrb_outpupt_reg;
always@(posedge s00_axi_aclk)begin 
	if ( rst ) bram_addrb_outpupt_reg <= 0;
	else if (kernel_start_out) bram_addrb_outpupt_reg <= bram_addra_output;
	else bram_addrb_outpupt_reg <= bram_addrb_outpupt_reg;
end

wire bram_enb_output, bram_web_output;
wire [10:0] bram_addrb_output;
wire [20:0] bram_dinb_output, bram_doutb_output;
assign bram_enb_output = (kernel_start_out_delay) ? 1 : (pooling_we) ? 1 : 0;
assign bram_web_output = (kernel_start_out_delay) ? 1 : (pooling_we) ? 1 : 0;
assign bram_addrb_output = (kernel_start_out_delay) ? bram_addrb_outpupt_reg : (pooling_we) ? pooling_addr_write : 0;
assign bram_dinb_output = (kernel_start_out_delay == 1 ) ? relu_or_acc : 
							(pooling_we) ? pooling_out : 0;
wire [20:0] relu_or_acc;
assign relu_or_acc = (acc_times[input_channel_check_bit] == 1'b1) ? relu_out : acc_result;

wire [20:0] acc_result;
wire unsigned [5:0] relu_out;
assign acc_result = bram_douta_output + { {6{kernel_out[14]}}, kernel_out} ;
assign relu_out = (acc_result[20] == 1'b1) ? 6'b000000 : 
				  ( |acc_result[19:9] ) ? 6'b111111 :	//  acc_result[19:9] have 1
				  (acc_result[2] == 1'b1 && (&acc_result[8:3] == 0) ) ? // acc_result[8:3] at least one bit is 0
				  acc_result[8:3] + 1'b1 : acc_result[8:3] ;
			
//	pooling====================================================================
reg pooling_en, pooling_we, pooling_done;
reg [11:0] pooling_addr_read;
wire [9:0] pooling_addr_write;
reg [5:0] pooling_fp_out;
wire [5:0] pooling_out;
assign pooling_out = ( pooling_fp_out >= bram_douta_output ) ? pooling_fp_out : bram_douta_output;
assign pooling_addr_write = (pooling_addr_read >> 1) - 1;
always @(posedge s00_axi_aclk) pooling_fp_out <= bram_douta_output;
always @(posedge s00_axi_aclk) begin 
	if ( rst ) pooling_done <= 0;
	//else if (pooling_addr_read == pooling_max_address) pooling_done <= 1 ;
	else if ( pooling_addr_read[pooling_max_check_bit] == 1'b1 ) pooling_done <= 1 ;
end

always @(posedge s00_axi_aclk) begin 
	if ( rst ) pooling_addr_read <= 0;
	//else if ( pooling_addr_read == pooling_max_address ) pooling_addr_read <= pooling_addr_read;
	else if ( pooling_addr_read[pooling_max_check_bit] == 1'b1 ) pooling_addr_read <= pooling_addr_read;
	else if ( pooling_en ) pooling_addr_read <= pooling_addr_read + 1'b1;
	else pooling_addr_read <= pooling_addr_read;
end


always @(posedge s00_axi_aclk) begin 
	if (rst) pooling_en <= 0;
	else if ( conv_and_relu_done && pooling_done == 0) pooling_en <= 1;
	//else if ( pooling_addr_read == pooling_max_address ) pooling_en <= 0;
	else if ( pooling_addr_read[pooling_max_check_bit] == 1'b1 ) pooling_en <= 0;
	else pooling_en <= pooling_en;
end

always @(posedge s00_axi_aclk) begin 
	if ( rst ) pooling_we <= 0;
	else if ( pooling_addr_read[0] == 1 ) pooling_we <= 1;
	else pooling_we <= 0;
end


blk_mem_gen_output bram_output(
	.clka(s00_axi_aclk), 
	.addra(bram_addra_output), 
	.dina(bram_dina_output), 
	.douta(bram_douta_output), 
	.wea(bram_wea_output), 
	.ena(bram_ena_output),
	
	.clkb(s00_axi_aclk),
	.addrb(bram_addrb_output),
	.dinb(bram_dinb_output),
	.doutb(bram_doutb_output),
	.web(bram_web_output),
	.enb(bram_enb_output)
) ; 

//====================================================================================================================
wire signed [14:0] kernel_out;
wire kernel_start_out, kernel_end_out;
wire [5:0] kernel_inputdata;
assign kernel_inputdata = bram_input_douta;
wire [8:0] kernel_output_num;

reg kernel_start_out_delay;
always@(posedge s00_axi_aclk) kernel_start_out_delay <= kernel_start_out;
Kernel mykernel(
 	.clk(s00_axi_aclk), 
	.op_st(op_st), 
	.rst(rst),
	.mode(mode),
	
	.in_data(kernel_inputdata), 
	.in_w(bram_doutw),
	
	.conv_and_relu_done(conv_and_relu_done),
	.in_vector_size_check_bit(in_vector_size_check_bit),
	
	.dout(kernel_out),
	.output_num(kernel_output_num),
	.start_out(kernel_start_out),
	.end_out(kernel_end_out)
	
);


assign done = pooling_done;
assign dout_output = { {11{bram_douta_output[20]}}, bram_douta_output};

// User logic ends

endmodule

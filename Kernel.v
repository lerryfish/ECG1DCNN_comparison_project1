
module Kernel 
( 	input wire clk, 
	input wire op_st, 
	input wire rst,
	input wire [1:0] mode,	
	
	input wire unsigned [5:0] in_data, 
	input wire [63:0] in_w,
	input wire [3:0] in_vector_size_check_bit,
	input wire conv_and_relu_done,	// myip tell kernel conv_and_relu_done
	
	output reg signed [14:0] dout,
	output reg [8:0] output_num,
	output reg start_out,
	output reg end_out
);

//enable ==========================================================================
reg en;	
always @(posedge clk or posedge rst) begin 
	if ( rst || conv_and_relu_done ) en <= 1'b0;
	else if ( op_st ) en <= 1'b1;
	else en <= en;
end

//weight ==========================================================================
wire signed [3:0] weight0, weight1, weight2, weight3, weight4, weight5, weight6, weight7;
wire signed [3:0] weight8, weight9, weight10, weight11, weight12, weight13, weight14, weight15;
assign weight0 = in_w[3:0];
assign weight1 = in_w[7:4];
assign weight2 = in_w[11:8];
assign weight3 = in_w[15:12];
assign weight4 = in_w[19:16];
assign weight5 = in_w[23:20];
assign weight6 = in_w[27:24];
assign weight7 = in_w[31:28];
assign weight8 = in_w[35:32];
assign weight9 = in_w[39:36];
assign weight10 = in_w[43:40];
assign weight11 = in_w[47:44];
assign weight12 = in_w[51:48];
assign weight13 = in_w[55:52];
assign weight14 = in_w[59:56];
assign weight15 = in_w[63:60];
	
// conv ==========================================================================
//	conv1 input data is 6 bit signed number,
//	conv2 conv3 conv4 input data are unsigned 6 bit
reg signed [6:0] data0, data1, data2, data3, data4, data5, data6, data7, data8, data9, data10, data11, data12, data13, data14, data15;
(* use_dsp = "yes" *)	reg signed [10:0] m0, m1, m2, m3, m4, m5, m6, m7, m8, m9, m10, m11, m12, m13, m14, m15;				
reg signed [12:0] p0 , p1, p2, p3;
reg [8:0] cnt;
always @(posedge clk or posedge rst) begin 
	if ( rst || end_out ) cnt <= 0;
	else if ( en == 1 && end_out == 0 ) begin 
		cnt <= cnt + 1;
	end
	else cnt <= cnt;
end

always @ ( posedge clk or posedge rst ) begin 
	if ( rst || end_out ) dout <= 15'd0;
	
	else if ( en == 1 && end_out == 0 ) begin 
		data0 <= data1;
		data1 <= data2;
		data2 <= data3;
		data3 <= data4;
		data4 <= data5;
		data5 <= data6;
		data6 <= data7;
		data7 <= data8;
		data8 <= data9;
		data9 <= data10;
		data10 <= data11;
		data11 <= data12;
		data12 <= data13;
		data13 <= data14;
		data14 <= data15;
		if ( cnt <= 6 ) data15 <= 0;
		else if (mode == 2'b00 && cnt >=263 ) data15 <= 0;
		else if (mode == 2'b01 && cnt >=135 ) data15 <= 0;
		else if (mode == 2'b10 && cnt >=71 ) data15 <= 0;
		else if (mode == 2'b11 && cnt >=39 ) data15 <= 0;
		else data15 <= (mode == 2'b00 ) ? {in_data, 1'b0} : {1'b0, in_data} ;	//7 bit
		// *******************
		
		m0 <= data0 * weight0 ; 	// 11 bit
		m1 <= data1 * weight1 ;
		m2 <= data2 * weight2 ;
		m3 <= data3 * weight3 ;
		m4 <= data4 * weight4 ;
		m5 <= data5 * weight5 ;
		m6 <= data6 * weight6 ;
		m7 <= data7 * weight7 ;
		m8 <= data8 * weight8 ;
		m9 <= data9 * weight9 ;
		m10 <= data10 * weight10 ;
		m11 <= data11 * weight11 ;
		m12 <= data12 * weight12 ;
		m13 <= data13 * weight13 ;
		m14 <= data14 * weight14 ;
		m15 <= data15 * weight15 ;
		// *******************
		
		p0 <= m0 + m1 + m2 + m3 ;	// 13 bit
		p1 <= m4 + m5 + m6 + m7 ;
		p2 <= m8 + m9 + m10 + m11 ;
		p3 <= m12 + m13 + m14 + m15 ;
		// *******************
		
		dout <= p0 + p1 + p2 + p3;	//15 bit
	end
end

// start_out & end_out & output_num==========================================================================
always @(posedge clk or posedge rst) begin 
	if ( rst || end_out) output_num <= 1;
	else if ( output_num[in_vector_size_check_bit] == 1'b1 ) output_num <= output_num;	//this line can remove
	else if ( start_out ) output_num <= output_num + 1;
	else output_num <= output_num;
end

always @ ( posedge clk or posedge rst ) begin
	if ( rst || end_out || conv_and_relu_done) start_out <= 0;
	else if ( output_num[in_vector_size_check_bit] == 1'b1 ) start_out <= 0;
	else if ( cnt == 17 ) start_out <= 1;
end

always @(posedge clk or posedge rst) begin 
	if ( rst || end_out ) end_out <= 0;
	else if ( output_num[in_vector_size_check_bit] == 1'b1 ) end_out <= 1;
	else end_out <= 0;
end

endmodule
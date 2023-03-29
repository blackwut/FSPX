#include "ap_int.h"
#include "ap_axi_sdata.h"
#include "hls_stream.h"

#include "../../include/fspx.hpp"


void test(
    fx::stream<int> & in,
    fx::stream<int> & out,
	int size
);

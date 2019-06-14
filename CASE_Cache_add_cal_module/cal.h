#pragma once

#include <iostream>

class Calculate {
private:
	double a;//
	double a_11;//scale 3
	double a_12; //scale 2
	double a_15; //scale 1		scale 0

private:
	// private functions
	long OEFUpdate(long c, long l, double a);
	double FC_OEF(long c, double a);
	double INV_FC_OEF(double n, double a);
	long OEF_renor(double a1, double a2, long c1);// previous a1,c1;next a2,c2;

public:
	void init();
	void update(long int *symb_value, int ByteCnt, int *scale_value);
	long backToRealValue(int symb_value, int scale);
};

// init 4 compression parameter
void Calculate::init() {
	a = pow(0.5, 11.730474);
	a_11 = pow(0.5, 11.730474);
	a_12 = pow(0.5, 12.0);
	a_15 = pow(0.5, 15.0);
}

long Calculate::backToRealValue(int symb_value, int scale) {
	long esti_value;
	if (scale == 0)
	{
		esti_value = symb_value;
	}
	else if (scale == 1)
	{
		esti_value = FC_OEF(symb_value, a_15);//FC_OEFÊÇËã»ØÀ´Âð
	}
	else if (scale == 2)
	{
		esti_value = FC_OEF(symb_value, a_12);
	}
	else if (scale == 3)
	{
		esti_value = FC_OEF(symb_value, a_11);
	}
	return esti_value;
}

void Calculate::update(long int *symb_value, int ByteCnt, int *scale_value) {
	if (*scale_value == 0) {
		*symb_value += ByteCnt;
		if (*symb_value > 65535) {
			*scale_value = 1;
			*symb_value = OEFUpdate(0, *symb_value, a_15);
		}
	}
	else if (*scale_value == 1) {
		*symb_value += OEFUpdate(*symb_value, (long)ByteCnt, a_15);
		if (*symb_value > 209345)
		{
			*scale_value = 2;
			*symb_value = OEF_renor(a_15, a_12, *symb_value);
		}
	}
	else if (*scale_value == 2) {
		*symb_value += OEFUpdate(*symb_value, (long)ByteCnt, a_12);
		if (*symb_value > 36322063329)
		{
			*scale_value = 3;
			*symb_value = OEF_renor(a_12, a_11, *symb_value);
		}
	}
	else if (*scale_value == 3)//a_11
	{
		*symb_value += OEFUpdate(*symb_value, (long)ByteCnt, a_11);
	}
}

long Calculate::OEFUpdate(long c, long l, double a) {
	double v = (double)(rand() / (double)RAND_MAX);

	double f_c = FC_OEF(c, a);//f(c)
	double delta = INV_FC_OEF((double)l + f_c, a) - c;
	delta = floor(delta);//delta
	double f_c_d = FC_OEF(c + (long)delta, a);//f(c+d);
	double f_c_d_1 = FC_OEF(c + (long)delta + 1, a);//f(c+d+1);
	double temp_1 = (double)l + f_c - f_c_d;
	double temp_2 = f_c_d_1 - f_c_d;
	double pd = temp_1 / temp_2;

	if (v <= pd)
		return (long)(delta + 1);
	else
		return (long)(delta);
}

double Calculate::FC_OEF(long c, double a) {
	double x = (pow((1 + a), (double)c) - 1) / a*(2 + a) / 2;
	return x;
}

double Calculate::INV_FC_OEF(double n, double a) {
	double temp_1 = log(2 * a*n + 2 + a) - log(2 + a);
	double temp_2 = log(1 + a);
	double x = temp_1 / temp_2;
	return x;
}

long Calculate::OEF_renor(double a1, double a2, long c1) {
	double v = (double)(rand() / (double)RAND_MAX);//generate a random number 0<x<1
	long c2;
	double c2_temp;
	c2_temp = log(a2*(2 + a1) / a1 / (2 + a2)*(pow(1 + a1, (double)c1) - 1) + 1.0) / log(1 + a2);
	double pd = c2_temp - (long)c2_temp;
	if (v <= pd)
	{
		c2 = (long)(c2_temp + 1);
	}
	else
	{
		c2 = (long)c2_temp;
	}
	return c2;
}
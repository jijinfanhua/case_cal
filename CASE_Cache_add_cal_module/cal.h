#pragma once

#include <iostream>
#include <map>
#define COMPRESS 1
#define PRINT_CAL 1

typedef long int llint;
using namespace std;

class Calculate {
private:
	double a;//
	double a_11;//scale 3
	double a_12; //scale 2
	double a_15; //scale 1		scale 0

#ifdef PRINT_CAL
	map<llint, double> f_c_container;
	map<double, double> inv_f_c_container;
	map<double, double> renor_a1_a2_container;
	map<double, llint> renor_a2_c1_container;
	FILE * oefupdate_fp;
	FILE * update_fp;
#endif
	
private:
	// private functions
	llint OEFUpdate(llint c, llint l, double a);
	double FC_OEF(llint c, double a);
	double INV_FC_OEF(double n, double a);
	llint OEF_renor(double a1, double a2, llint c1);// previous a1,c1;next a2,c2;

public:
	void init();
	void update(llint *symb_value, int ByteCnt, int *scale_value);
	llint backToRealValue(llint symb_value, int scale);
#ifdef PRINT_CAL
	void off_file();
	void write_cal_to_file();
#endif
};

// init 4 compression parameter
void Calculate::init() {
	a = pow(0.5, 11.730474);
	a_11 = pow(0.5, 11.730474);
	a_12 = pow(0.5, 12.0);
	a_15 = pow(0.5, 15.0);
#ifdef PRINT_CAL
	oefupdate_fp = fopen("oefupdate.txt", "w");
#endif
}

llint Calculate::backToRealValue(llint symb_value, int scale) {
#ifdef COMPRESS
	llint esti_value;
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
#else
	return symb_value;
#endif
}

void Calculate::update(llint *symb_value, int ByteCnt, int *scale_value) {
#ifdef PRINT_CAL
	fprintf(update_fp, "%lld\t%d\t%d\n", *symb_value, ByteCnt, *scale_value);
#endif
#ifdef COMPRESS
	if (*scale_value == 0) {
		*symb_value += ByteCnt;
		if (*symb_value > 65535) {
			*scale_value = 1;
			*symb_value = OEFUpdate(0, *symb_value, a_15);
		}
	}
	else if (*scale_value == 1) {
		*symb_value += OEFUpdate(*symb_value, (llint)ByteCnt, a_15);
		if (*symb_value > 209345)
		{
			*scale_value = 2;
			*symb_value = OEF_renor(a_15, a_12, *symb_value);
		}
	}
	else if (*scale_value == 2) {
		*symb_value += OEFUpdate(*symb_value, (llint)ByteCnt, a_12);
		if (*symb_value > 36322063329)
		{
			*scale_value = 3;
			*symb_value = OEF_renor(a_12, a_11, *symb_value);
		}
	}
	else if (*scale_value == 3)//a_11
	{
		*symb_value += OEFUpdate(*symb_value, (llint)ByteCnt, a_11);
	}
	fprintf(update_fp, "%lld\t%d\n", *symb_value, *scale_value);
	return;
#else
	*symb_value += ByteCnt;
	return;
#endif
}

llint Calculate::OEFUpdate(llint c, llint l, double a) {
	fprintf(oefupdate_fp, "%lld\t%lld\t%.10lf\n", c, l, a);
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

double Calculate::FC_OEF(llint c, double a) {
#ifdef PRINT_CAL
	f_c_container.insert(make_pair(c, a));
#endif
	double x = (pow((1 + a), (double)c) - 1) / a*(2 + a) / 2;
	return x;
}

double Calculate::INV_FC_OEF(double n, double a) {
#ifdef PRINT_CAL
	inv_f_c_container.insert(make_pair(n, a));
#endif
	double temp_1 = log(2 * a*n + 2 + a) - log(2 + a);
	double temp_2 = log(1 + a);
	double x = temp_1 / temp_2;
	return x;
}

llint Calculate::OEF_renor(double a1, double a2, llint c1) {
#ifdef PRINT_CAL
	renor_a1_a2_container.insert(make_pair(a1, a2));
	renor_a2_c1_container.insert(make_pair(a2, c1));
#endif
	double v = (double)(rand() / (double)RAND_MAX);//generate a random number 0<x<1
	llint c2;
	double c2_temp;
	c2_temp = log(a2*(2 + a1) / a1 / (2 + a2)*(pow(1 + a1, (double)c1) - 1) + 1.0) / log(1 + a2);
	double pd = c2_temp - (long)c2_temp;
	if (v <= pd)
	{
		c2 = (llint)(c2_temp + 1);
	}
	else
	{
		c2 = (llint)c2_temp;
	}
	return c2;
}

#ifdef PRINT_CAL
void Calculate::write_cal_to_file() {
	/* write f_c table */
	FILE * fp = fopen("f_c.txt", "w");
	map<llint, double>::iterator it1;
	it1 = f_c_container.begin();
	while (it1 != f_c_container.end()) {
		fprintf(fp, "%lld\t%lf\n", it1->first, it1->second);
		it1++;
	}
	fclose(fp);

	/* write inverse f_c table */
	fp = fopen("inv_f_c.txt", "w");
	map<double, double>::iterator it2;
	it2 = inv_f_c_container.begin();
	while (it2 != inv_f_c_container.end()) {
		fprintf(fp, "%lf\t%lf\n", it2->first, it2->second);
		it2++;
	}
	fclose(fp);

	/* write renor a1_a2 table */
	fp = fopen("renor_a1_a2.txt", "w");
	map<double, double>::iterator it3;
	it3 = renor_a1_a2_container.begin();
	while (it3 != renor_a1_a2_container.end()) {
		fprintf(fp, "%lf\t%lf\n", it3->first, it3->second);
		it3++;
	}
	fclose(fp);

	/* write renor a1_a2 table */
	fp = fopen("renor_a2_c1.txt", "w");
	map<double, llint>::iterator it4;
	it4 = renor_a2_c1_container.begin();
	while (it4 != renor_a2_c1_container.end()) {
		fprintf(fp, "%lf\t%lf\n", it4->first, it4->second);
		it4++;
	}
	fclose(fp);
}
#endif

#ifdef PRINT_CAL
void Calculate::off_file() {
	fclose(oefupdate_fp);
	fclose(update_fp);
}
#endif
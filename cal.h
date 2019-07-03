#pragma once

#ifndef CAL_h
#define CAL_h

#include <iostream>
#include <map>
#include <cmath>

#include "A_11.h"
#include "A_12.h"
#include "A_15.h"

#define COMPRESS 1
//#define PRINT_CAL 1
//#define M128_U32 1

typedef long int llint;
typedef unsigned int uint;

//define the flow_id type is (unsigned int)
//		 the ByteCnt type is (unsigned long)
//		 the symbValue type is (unsigned long)

typedef unsigned int case_flowid_t;
typedef unsigned long case_bytecnt_t;
typedef unsigned long case_symb_t;

using namespace std;

class Calculate {
private:
	double a;//
	double a_11;//scale 3
	double a_12; //scale 2
	double a_15; //scale 1		scale 0
	A_15 *A15;
	A_12 *A12;
	A_11 *A11;

#ifdef PRINT_CAL
	map<case_symb_t, double> f_c_container;
	map<double, double> inv_f_c_container;
	map<double, double> renor_a1_a2_container;
	map<double, case_symb_t> renor_a2_c1_container;
	FILE * oefupdate_fp;
	FILE * update_fp;
#endif
	
private:
	// private functions
	case_symb_t OEFUpdate(case_symb_t c, case_bytecnt_t l, double a, int level);
	double FC_OEF(case_symb_t c, double a, int level);
	double INV_FC_OEF(double n, double a, int level);
	case_symb_t OEF_renor(double a1, double a2, case_symb_t c1);// previous a1,c1;next a2,c2;

public:
	void init();
	void update(case_symb_t *symb_value, case_bytecnt_t ByteCnt, int *scale_value);
#ifdef COMPRESS
	double backToRealValue(case_symb_t symb_value, int scale);
#else
	case_symb_t backToRealValue(case_symb_t symb_value, int scale);
#endif

#ifdef PRINT_CAL
	void off_file();
	void write_cal_to_file();
#endif
};

// init 4 compression parameter
void Calculate::init() {
	A15 = new A_15();
	A12 = new A_12();
	A11 = new A_11();
	A15->init();
	A12->init();
	A11->init();

	a = pow(0.5, 11.730474);
	a_11 = pow(0.5, 11.730474);//level 3
	a_12 = pow(0.5, 12.0);//level 2
	a_15 = pow(0.5, 15.0);//level 1
#ifdef PRINT_CAL
	oefupdate_fp = fopen("oefupdate.txt", "w");
	update_fp = fopen("update.txt", "w");
#endif
}

#ifdef COMPRESS
double Calculate::backToRealValue(case_symb_t symb_value, int scale) {
	double esti_value;
	if (scale == 0)
	{
		esti_value = symb_value;
	}
	else if (scale == 1)
	{
		esti_value = FC_OEF(symb_value, a_15, 1);//FC_OEFÊÇËã»ØÀ´Âð
	}
	else if (scale == 2)
	{
		esti_value = FC_OEF(symb_value, a_12, 2);
	}
	else if (scale == 3)
	{
		esti_value = FC_OEF(symb_value, a_11, 3);
	}
	return esti_value;
}
#else
case_symb_t Calculate::backToRealValue(case_symb_t symb_value, int scale) {
	return symb_value;
}
#endif

void Calculate::update(case_symb_t *symb_value, case_bytecnt_t ByteCnt, int *scale_value) {
#ifdef PRINT_CAL
	fprintf(update_fp, "%lu\t%lu\t%d\n", *symb_value, ByteCnt, *scale_value);
#endif
#ifdef COMPRESS
	if (*scale_value == 0) {
		*symb_value += ByteCnt;
		if (*symb_value > 65535) {
			*scale_value = 1;
			*symb_value = OEFUpdate(0, *symb_value, a_15, 1);
		}
	}
	else if (*scale_value == 1) {
		*symb_value += OEFUpdate(*symb_value, ByteCnt,a_15, 1);
		if (*symb_value > 209345)
		{
			*scale_value = 2;
			*symb_value = OEF_renor(a_15, a_12, *symb_value);
		}
	}
	else if (*scale_value == 2) {
		*symb_value += OEFUpdate(*symb_value, ByteCnt, a_12, 2);
		if (*symb_value > 36322063329)
		{
			*scale_value = 3;
			*symb_value = OEF_renor(a_12, a_11, *symb_value);
		}
	}
	else if (*scale_value == 3)//a_11
	{
		*symb_value += OEFUpdate(*symb_value, ByteCnt, a_11, 3);
	}
#ifdef PRINT_CAL
	fprintf(update_fp, "%lu\t%d\n", *symb_value, *scale_value);
#endif
	return;
#else
	*symb_value += ByteCnt;
	return;
#endif
}

case_symb_t Calculate::OEFUpdate(case_symb_t c, case_bytecnt_t l, double a, int level) {
#ifdef PRINT_CAL
	fprintf(oefupdate_fp, "%lu\t%lu\t%.10lf\n", c, l, a);
#endif
	double v = (double)(rand() / (double)RAND_MAX);

	double f_c = FC_OEF(c, a, level);//f(c)
	double delta = INV_FC_OEF((double)l + f_c, a, level) - c;
	delta = floor(delta);//delta
	double f_c_d = FC_OEF(c + (case_symb_t)delta, a, level);//f(c+d);
	double f_c_d_1 = FC_OEF(c + (case_symb_t)delta + 1, a, level);//f(c+d+1);
	double temp_1 = (double)l + f_c - f_c_d;
	double temp_2 = f_c_d_1 - f_c_d;
	double pd = temp_1 / temp_2;

	if (v <= pd)
		return (case_symb_t)(delta + 1);
	else
		return (case_symb_t)(delta);
}

double Calculate::FC_OEF(case_symb_t c, double a, int level) {
#ifdef PRINT_CAL
	f_c_container.insert(make_pair(c, a));
#endif
	double x;
	if(c <= 201460){
		if(level == 1){
			x = A15->a[c];
		}else if(level == 2){
			x = A12->a[c];
		}else if(level == 3){
			x = A11->a[c];
		}
	}else{
		x = (pow((1 + a), (double)c) - 1) / a*(2 + a) / 2;
	}
	//double x = (pow((1 + a), (double)c) - 1) / a*(2 + a) / 2;
	return x;
}

double Calculate::INV_FC_OEF(double n, double a, int level) {
#ifdef PRINT_CAL
	inv_f_c_container.insert(make_pair(n, a));
#endif
	double temp_1 = log(2 * a*n + 2 + a) - log(2 + a);
	double temp_2 = log(1 + a);
	double x = temp_1 / temp_2;
	return x;
}

case_symb_t Calculate::OEF_renor(double a1, double a2, case_symb_t c1) {
#ifdef PRINT_CAL
	renor_a1_a2_container.insert(make_pair(a1, a2));
	renor_a2_c1_container.insert(make_pair(a2, c1));
#endif
	double v = (double)(rand() / (double)RAND_MAX);//generate a random number 0<x<1
	case_symb_t c2;
	double c2_temp;
	c2_temp = log(a2*(2 + a1) / a1 / (2 + a2)*(pow(1 + a1, (double)c1) - 1) + 1.0) / log(1 + a2);
	double pd = c2_temp - (case_symb_t)c2_temp;
	if (v <= pd)
	{
		c2 = (case_symb_t)(c2_temp + 1);
	}
	else
	{
		c2 = (case_symb_t)c2_temp;
	}
	return c2;
}

#ifdef PRINT_CAL
void Calculate::write_cal_to_file() {
	/* write f_c table */
	FILE * fp = fopen("f_c.txt", "w");
	map<case_symb_t, double>::iterator it1;
	it1 = f_c_container.begin();
	while (it1 != f_c_container.end()) {
		fprintf(fp, "%lu\t%lf\n", it1->first, it1->second);
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
	map<double, case_symb_t>::iterator it4;
	it4 = renor_a2_c1_container.begin();
	while (it4 != renor_a2_c1_container.end()) {
		fprintf(fp, "%lf\t%lu\n", it4->first, it4->second);
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

#endif

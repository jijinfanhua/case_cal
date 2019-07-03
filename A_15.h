
#ifndef A_15_H
#define A_15_H

class A_15{
public:
	double a[209461];
	void init();
};

void A_15::init(){
	FILE * fp = fopen("a_15.txt","r");

	for(int i = 0; i < 201461; i++){
		fscanf(fp, "%lf", &a[i]);
	}

	fclose(fp);
}


#endif
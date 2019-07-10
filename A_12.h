
#ifndef A_12_H
#define A_12_H

class A_12{
public:
	double a[209461];
	void init();
};

void A_12::init(){
	FILE * fp = fopen("a_12.txt","r");

	for(int i = 0; i < 201461; i++){
		fscanf(fp, "%lf", &a[i]);
	}

	fclose(fp);
}


#endif
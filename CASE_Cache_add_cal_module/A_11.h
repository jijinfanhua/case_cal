
#ifndef A_11_H
#define A_11_H

class A_11{
public:
	double a[209461];
	void init();
};

void A_11::init(){
	FILE * fp = fopen("a_11.txt","r");

	for(int i = 0; i < 201461; i++){
		fscanf(fp, "%lf", &a[i]);
	}

	fclose(fp);
}


#endif
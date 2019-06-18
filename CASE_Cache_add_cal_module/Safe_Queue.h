#ifndef SAFE_QUEUE
#define SAFE_QUEUE

#define MAX_NUMBER 10000000
#define OK     0
#define NOTOK -1

template<class T>
class QUEUE_DATA {
private:
    T data[MAX_NUMBER];
    int head;
    int tail;
public:
    QUEUE_DATA() {
        this->head = 0;
        this->tail = 0;
    }
    ~QUEUE_DATA() {}

    int push_data(T data) {
        if(this->head == ((this->tail) + 1)% MAX_NUMBER)
            return NOTOK;

        this->data[this->tail] = data;
        this->tail = (this->tail + 1)% MAX_NUMBER;
        return OK;
    }

    int pop_data(T* pData) {
        /*if(NULL == pData)
            return NOTOK;*/

		if(this->head == this->tail) {
			return NOTOK; 
		}
            

        *pData = this->data[this->head];
        this->head = (this->head + 1)% MAX_NUMBER;
        return OK;
    }

};
#endif // SAFE_QUEUE

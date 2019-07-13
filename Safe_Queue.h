#ifndef SAFE_QUEUE
#define SAFE_QUEUE

#define MAX_NUMBER 10000000
#define OK     0
#define NOTOK -1

template<class T>
class QUEUE_DATA {
private:
    //T data[MAX_NUMBER];
	T* data;
    int head;
    int tail;
	int max_length;
public:
    QUEUE_DATA() {
        this->head = 0;
        this->tail = 0;
		this->max_length = MAX_NUMBER;
		this->data = new T[max_length];
    }
	QUEUE_DATA(long queue_size) {
		this->head = 0;
        this->tail = 0;
		this->max_length = queue_size;
		this->data = new T[queue_size];
	}
	QUEUE_DATA(const QUEUE_DATA &queue_data) {
		this->head = queue_data->head;
        this->tail = queue_data->tail;
		this->max_length = queue_data->max_length;
		this->data = new T[max_length];
		memcpy(this->data, queue_data->data,queue_data->max_length*sizeof(T));
	}
    ~QUEUE_DATA() {
		delete[] this->data;
	}

    int push_data(T data) {
        if(this->head == ((this->tail) + 1)% this->max_length)
            return NOTOK;

        this->data[this->tail] = data;
        this->tail = (this->tail + 1)% this->max_length;
        return OK;
    }

    int pop_data(T* pData) {
        /*if(NULL == pData)
            return NOTOK;*/

		if(this->head == this->tail) {
			return NOTOK; 
		}
            

        *pData = this->data[this->head];
        this->head = (this->head + 1)% this->max_length;
        return OK;
    }

	int queue_size() {
		return (this->tail + this->max_length - this->head) % this->max_length;
	}

};
#endif // SAFE_QUEUE

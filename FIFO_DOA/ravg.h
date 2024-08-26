#ifndef __RAVG_H__
#define __RAVG_H__

#include <stdlib.h>

template <class T, class S>
class Ravg
{
public:
	// Constructor
	Ravg(const unsigned int N)
	{
		N_elems = N;
		avg = 0.0f;
		tail = 0;

		array = (T*) malloc(sizeof(T) * N_elems);
		if (!array) {
			exit(1);
		}

		for (int i = 0; i < N_elems; ++i) {
			array[i] = T();
		}
	}

	// TODO: copy constructor

	// Destructor
	~Ravg()
	{
		if (array) {
			free(array);
		}
	}

	// insert a new value and return the average
	S insert(const T elem)
	{
		avg -= array[tail] / N_elems;
		avg += elem / N_elems;

		array[tail] = elem;
		if (++tail == N_elems) {
			tail = 0;
		}
		return avg;
	}

	// return the average
	S get_avg()
	{
		return avg;
	}

private:
	unsigned int N_elems;
	unsigned int tail;
	S avg;
	T* array;

};


#endif /* __RAVG_H__ */

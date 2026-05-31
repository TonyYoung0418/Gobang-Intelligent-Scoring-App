#ifndef STACK_GUARD
#define STACK_GUARD
#include "tools.h"
//状态栈类，与std::stack的区别在于数据默认是存在运行栈中的，这样就不用动态分配内存了，
//从而提高了程序的运行效率。
template<typename T, int MAXSIZE>
class stack {
private:
	T data[MAXSIZE];
	T *ptr = data;
public:
	inline T *begin() {
		return data;
	}
	inline T *end() {
		return ptr;
	}
	inline const T *begin() const {
		return data;
	}
	inline const T *end() const {
		return ptr;
	}
	inline size_t size() const {
		return ptr - data;
	}
	inline bool empty() const {
		return ptr == data;
	}
	inline T &expand() {
#ifndef FINAL_RELEASE
		if (ptr == data + MAXSIZE)
			error("expand时stack溢出。");
#endif
		return *ptr++;
	}
	inline void pop() {
#ifndef FINAL_RELEASE
		if (ptr == data)
			error("试图对空stack调用pop操作");
#endif
		--ptr;
	}
	inline T &top() {
#ifndef FINAL_RELEASE
		if (ptr == data)
			error("试图对空stack调用top操作");
#endif
		return *(ptr - 1);
	}
	inline const T &top() const {
#ifndef FINAL_RELEASE
		if (ptr == data)
			error("试图对空stack调用top操作");
#endif
		return *(ptr - 1);
	}
	inline void push(const T &sth) {
#ifndef FINAL_RELEASE
		if (ptr == data + MAXSIZE)
			error("push时stack溢出。");
#endif
		*ptr++ = sth;
	}
	inline T &operator[] (int idx) {
		return data[idx];
	}
	inline const T &operator[] (int idx) const {
		return data[idx];
	}
	inline void clear() {
		ptr = data;
	}
};
#endif
#include <iostream>
#include <cstdint>

using std::size_t;

template<size_t I, typename T>
struct Holder{
	T value;
};

template<typename T, size_t I>
decltype(auto) get(const Holder<I,T>& h){
	return h.value;
}

template<typename T, size_t I>
decltype(auto) get(Holder<I,T>& h){
	return h.value;
}

template<size_t I,typename T>
decltype(auto) get(const Holder<I,T>& h){
	return h.value;
}

template<size_t I,typename T>
decltype(auto) get(Holder<I,T>& h){
	return h.value;
}

struct null_t{};

template<typename T0 = null_t, typename T1 = null_t, typename T2 = null_t>
struct tuple;


template<typename T0>
struct tuple<T0>:Holder<0,T0>{
	tuple() = default;
	tuple(T0 t0):Holder<0,T0>(std::move(t0)){}
};

template<typename T0,typename T1>
struct tuple<T0,T1>:Holder<0,T0>, Holder<1,T1>{
	tuple() = default;
	tuple(T0 t0,T1 t1):Holder<0,T0>(std::move(t0)),Holder<1,T1>(std::move(t1)){}
};


int main(){
	tuple<int> t{1};
	std::cout << get<int>(t) << "\n";

	tuple<int,double> t2{1,3.5};
	std::cout << get<double>(t2) << "\n";
	std::cout << get<0>(t2) << "\n";
	std::cout << get<1>(t2) << "\n";

}
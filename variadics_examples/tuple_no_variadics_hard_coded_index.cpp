#include <iostream>

template<typename T>
struct Holder{
	T value;
};

template<typename T>
decltype(auto) get(const Holder<T>& h){
	return h.value;
}

template<typename T>
decltype(auto) get(Holder<T>& h){
	return h.value;
}

struct null_t{};

template<typename T0 = null_t, typename T1 = null_t, typename T2 = null_t>
struct tuple;


template<typename T0>
struct tuple<T0>:Holder<T0>{
	tuple() = default;
	tuple(T0 t0):Holder<T0>(std::move(t0)){}
};

template<typename T0,typename T1>
struct tuple<T0,T1>:Holder<T0>, Holder<T1>{
	tuple() = default;
	tuple(T0 t0,T1 t1):Holder<T0>(std::move(t0)),Holder<T1>(std::move(t1)){}
};

int main(){
	tuple<int> t{1};
	std::cout << get<int>(t) << "\n";

	tuple<int,double> t2{1,3.5};
	std::cout << get<double>(t2) << "\n";

}
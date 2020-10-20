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

constexpr auto no_idx = static_cast<size_t>(-1);

template<size_t I0 = no_idx, size_t I1 = no_idx, size_t I2 = no_idx>
struct sequence{};

template<typename Sequence, typename T0 = null_t, typename T1 = null_t, typename T2 = null_t>
struct tuple_impl;


template<typename T0, size_t I0>
struct tuple_impl<sequence<I0>,T0>:Holder<I0,T0>{
	tuple_impl() = default;
	tuple_impl(T0 t0):Holder<I0,T0>{std::move(t0)}{}
};

template<typename T0>
struct tuple<T0>:tuple_impl<sequence<0>,T0>{
	using base = tuple_impl<sequence<0>,T0>;
	using base::base;
};

template<typename T0,typename T1, size_t I0, size_t I1>
struct tuple_impl<sequence<I0,I1>,T0,T1>:Holder<I0,T0>, Holder<I1,T1>{
	tuple_impl() = default;
	tuple_impl(T0 t0,T1 t1):Holder<I0,T0>{std::move(t0)},Holder<I1,T1>{std::move(t1)}{}
};

template<typename T0,typename T1>
struct tuple<T0,T1>:tuple_impl<sequence<0,1>,T0,T1>{
	using base = tuple_impl<sequence<0,1>,T0,T1>;
	using base::base;
};


int main(){
	tuple<int> t{1};
	std::cout << get<int>(t) << "\n";

	tuple<int,double> t2{1,3.5};
	std::cout << get<double>(t2) << "\n";
	std::cout << get<0>(t2) << "\n";
	std::cout << get<1>(t2) << "\n";

}
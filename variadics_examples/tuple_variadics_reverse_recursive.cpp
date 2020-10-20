#include <iostream>
#include <cstdint>
#include <utility>

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

template<typename Sequence, typename... Ts>
struct tuple_impl;

template<typename... Ts, size_t... Is>
struct tuple_impl<std::index_sequence<Is...>,Ts...>:Holder<Is,Ts>...{
	tuple_impl() = default;
	tuple_impl(Ts... ts):Holder<Is,Ts>{std::move(ts)}...{}
};

template<typename... Ts>
struct tuple:tuple_impl<std::make_index_sequence<sizeof...(Ts)>,Ts...>{
	using base = tuple_impl<std::make_index_sequence<sizeof...(Ts)>,Ts...>;
	using base::base;
};

template<typename Tuple, typename... Rs>
struct reverse_tuple;

template<typename... Rs>
struct reverse_tuple<tuple<>,Rs...>{
	using type = tuple<Rs...>;
};

template<typename First, typename... Ts, typename... Rs>
struct reverse_tuple<tuple<First,Ts...>,Rs...>{
	using type = typename reverse_tuple<tuple<Ts...>,First,Rs...>::type;
};


template<typename Tuple>
using reverse_tuple_t = typename reverse_tuple<Tuple>::type;


int main(){
	tuple<int> t{1};
	std::cout << get<int>(t) << "\n";

	tuple<int,double> t2{1,3.5};
	std::cout << get<double>(t2) << "\n";
	std::cout << get<0>(t2) << "\n";
	std::cout << get<1>(t2) << "\n";

	reverse_tuple_t<tuple<int,double>> t3{3.5,1};
	static_assert(std::is_same_v<tuple<double,int>,reverse_tuple_t<tuple<int,double>>>);
	std::cout << get<0>(t3) << "\n";
	std::cout << get<1>(t3) << "\n";



}
#include <iostream>
#include <type_traits>
#include <string>

struct AdGroup {
  int customer_id() const { return 5; }
  int campaign_id() const { return 5; }
  int ad_group_id() const { return 5; }
};

struct Campaign {
  int customer_id() const { return 5; }
  int campaign_id() const { return 5; }
};

struct Customer {
  int customer_id() const { return 5; }
};

template <typename T>
typename T::value_type get_value(const T& t) {
  return t();
}

int get_value(int i) { return i; }

template<typename bool,typename T>
struct enable_if{};

template<typename T>
struct enable_if<true, T>{
	using type = T;
};

template<bool b, typename T = void>
using enable_if_t = typename enable_if<b,T>::type;

template<typename T, enable_if_t<sizeof(T) < 4,bool> = true>
std::string Size(const T&){
	return "small";
}

template<typename T, enable_if_t<sizeof(T) >= 4,bool> = true>
std::string Size(const T&){
	return "big";
}



int main(){
	std::integral_constant<int, 2> ic;
	int i = 3;
	std::cout << get_value(ic) << " " << get_value(i) << "\n";
	std::cout << Size(1) << " " << Size('a') << "\n";

}

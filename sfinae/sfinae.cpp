#include <iostream>
#include <type_traits>

struct AdGroup {
  int ad_group_id() const { return 5; }
};

template <typename T>
typename T::value_type get_value(const T& t) {
  return t();
}

int get_value(int i) { return i; }


int main(){
	std::integral_constant<int, 2> ic;
	int i = 3;
	std::cout << get_value(ic) << " " << get_value(i);

}

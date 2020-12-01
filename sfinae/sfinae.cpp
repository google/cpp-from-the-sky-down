#include <iostream>
#include <string>
#include <type_traits>

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

template <bool, typename T>
struct enable_if {};

template <typename T>
struct enable_if<true, T> {
  using type = T;
};

template <bool b, typename T = void>
using enable_if_t = typename enable_if<b, T>::type;

template <typename T, enable_if_t<sizeof(T) < 4, bool> = true>
std::string Size(const T&) {
  return "small";
}

template <typename T, enable_if_t<sizeof(T) >= 4, bool> = true>
std::string Size(const T&) {
  return "big";
}

template <typename...>
using void_t = void;

template <typename T>
T declval();

template <typename T, typename = void>
struct HasCampaignId : std::false_type {};

template <typename T>
struct HasCampaignId<T, void_t<decltype(declval<T>().campaign_id())>>
    : std::true_type {};

template<typename T>
constexpr bool has_campaign_id(){
    return false;
}




template<typename T>
constexpr bool has_campaign_id()
requires requires(T t){t.campaign_id();}
{
    return true;
}


template<typename T>
constexpr bool has_ad_group_id(){
    if constexpr(requires(T t){t.ad_group_id();}){
        return true;
    }
  else{
        return false;
   }
}


int main() {
  std::integral_constant<int, 2> ic;
  int i = 3;
  std::cout << get_value(ic) << " " << get_value(i) << "\n";
  std::cout << Size(1) << " " << Size('a') << "\n";
  std::cout << HasCampaignId<Campaign>::value << " "
            << HasCampaignId<Customer>::value << "\n";
  std::cout << has_campaign_id<Campaign>() << " "
            << has_campaign_id<Customer>() << "\n";
  std::cout << has_ad_group_id<AdGroup>() << " "
            << has_ad_group_id<Customer>() << "\n";




}

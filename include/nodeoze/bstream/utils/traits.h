#ifndef UTILS_TRAITS_H
#define UTILS_TRAITS_H

#include <type_traits>
namespace nodeoze
{
namespace bstream
{
namespace utils
{
	template<class> struct sfinae_true_if : std::true_type {};

	template <bool... B>
	struct conjunction {};

	template <bool Head, bool... Tail>
	struct conjunction<Head, Tail...>
		: std::integral_constant<bool, Head && conjunction<Tail...>::value>{};

	template <bool B>
	struct conjunction<B> : std::integral_constant<bool, B> {};

} // namespace utils
} // namespace bstream
} // namespace nodeoze

#endif // UTILS_TRAITS_H

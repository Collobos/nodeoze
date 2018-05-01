#ifndef NODEOZE_TRAITS_H
#define NODEOZE_TRAITS_H

#include <type_traits>

namespace nodeoze
{

	template <class T, template <class...> class Template>
	struct is_specialization : std::false_type {};

	template <template <class...> class Template, class... Args>
	struct is_specialization<Template<Args...>, Template> : std::true_type {};

}

#endif

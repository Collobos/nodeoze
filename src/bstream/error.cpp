#include <nodeoze/bstream/error.h>

using namespace nodeoze;
using namespace bstream;

const char* 
bstream_category_impl::name() const noexcept
{
    return "nodeoze::bstream";
}

std::string 
bstream_category_impl::message(int ev ) const noexcept
{
    switch ( static_cast< errc > (ev ) )
    {
    case bstream::errc::ok:
        return "success";
    case bstream::errc::read_past_end_of_stream:
        return "read past end of stream";
    case bstream::errc::type_error:
        return "type error";
    case bstream::errc::member_count_error:
        return "member count error";
    case bstream::errc::context_mismatch:
        return "context mismatch";
    case bstream::errc::invalid_err_category:
        return "invalid error category";
    case bstream::errc::invalid_ptr_downcast:
        return "invalid pointer downcast";
    case bstream::errc::abstract_non_poly_class:
        return "abstract class not found in polymorphic context";
    case bstream::errc::invalid_operation:
        return "invalid operation";
    case bstream::errc::invalid_state:
        return "invalid state";
    case bstream::errc::ibstreambuf_not_shareable:
        return "bstream buffer is not shareable";
    default:
        return "unknown bstream error";
    }
}

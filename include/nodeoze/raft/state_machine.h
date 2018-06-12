#ifndef NODEOZE_RAFT_STATE_MACHINE_H
#define NODEOZE_RAFT_STATE_MACHINE_H

#include <functional>
#include <nodeoze/raft/types.h>
#include <nodeoze/buffer.h>

namespace nodeoze
{
namespace raft
{

class state_machine
{
public:

	using payload_type = buffer;

	using apply_result_func = std::function<void (payload_type&& result)>;

	virtual ~state_machine() {}

	virtual void 
	initialize( index_type index = 0 );

	virtual payload_type
	apply(payload_type const& update) = 0;
	
	virtual void
	apply(apply_result_func result_func, payload_type const& update) = 0;

};

} // namespace raft
} // namespace nodeoze

#endif // NODEOZE_RAFT_STATE_MACHINE_H

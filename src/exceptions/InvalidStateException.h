#ifndef STORM_EXCEPTIONS_INVALIDSTATEEXCEPTION_H_
#define STORM_EXCEPTIONS_INVALIDSTATEEXCEPTION_H_

#include "src/exceptions/BaseException.h"
#include "src/exceptions/ExceptionMacros.h"

namespace storm {
    namespace exceptions {
        
        STORM_NEW_EXCEPTION(InvalidStateException)
        
    } // namespace exceptions
} // namespace storm

#endif /* STORM_EXCEPTIONS_INVALIDSTATEEXCEPTION_H_ */

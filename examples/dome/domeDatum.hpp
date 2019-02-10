#ifndef OPENPOSE_EXAMPLES_DOME_DOME_DATUM_HPP
#define OPENPOSE_EXAMPLES_DOME_DOME_DATUM_HPP

// OpenPose dependencies
#include <openpose/headers.hpp>

// If the user needs his own variables, he can inherit the op::Datum struct and add them in there.
// DomeDatum can be directly used by the OpenPose wrapper because it inherits from op::Datum, just define
// WrapperT<std::vector<std::shared_ptr<DomeDatum>>> instead of Wrapper
// (or equivalently WrapperT<std::vector<std::shared_ptr<DomeDatum>>>)
struct DomeDatum : public op::Datum
{
    int panelIdx;
    int camIdx;

    // DomeDatum(const bool boolThatUserNeedsForSomeReason_ = false) :
    //     boolThatUserNeedsForSomeReason{boolThatUserNeedsForSomeReason_}
    // {}
};

#endif // OPENPOSE_EXAMPLES_DOME_DOME_DATUM_HPP

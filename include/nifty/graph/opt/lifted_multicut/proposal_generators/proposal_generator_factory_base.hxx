
#pragma once


#include <memory>


#include "nifty/graph/opt/lifted_multicut/proposal_generators/proposal_generator_base.hxx"

namespace nifty{
namespace graph{
namespace opt{
namespace lifted_multicut{


    template<class OBJECTIVE>
    class ProposalGeneratorFactoryBase{
    public:
        typedef OBJECTIVE ObjectiveType;
        typedef ProposalGeneratorBase<ObjectiveType> ProposalGeneratorBaseType;

        virtual ~ProposalGeneratorFactoryBase(){}
        virtual std::shared_ptr<ProposalGeneratorBaseType> createShared(const ObjectiveType & objective, const std::size_t numberOfThreads) = 0;
        virtual ProposalGeneratorBaseType *                create(   const ObjectiveType & objective, const std::size_t numberOfThreads) = 0;
    };



}
}
}
}


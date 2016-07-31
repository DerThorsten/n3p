#pragma once
#ifndef NIFTY_HDF5_HDF5_ARRAY
#define NIFTY_HDF5_HDF5_ARRAY

#include <string>
#include <vector>

#include "nifty/tools/runtime_check.hxx"
#include "nifty/marray/marray_hdf5.hxx"

namespace nifty{
namespace hdf5{
    
    using namespace marray::hdf5;




    template<class T>
    class Hdf5Array{
    public:
        // reading 
        Hdf5Array(
            const hid_t& groupHandle,
            const std::string & datasetName
        )
        :   groupHandle_(groupHandle),
            dataset_()
        {
            dataset_ = H5Dopen(groupHandle_, datasetName.c_str(), H5P_DEFAULT);
            if(dataset_ < 0) {
                throw std::runtime_error("Marray cannot open dataset.");
            }

            // select dataspace hyperslab
            datatype_ = H5Dget_type(dataset_);
            if(!H5Tequal(dataset_, hdf5Type<T>())) {
                throw std::runtime_error("data type of stored hdf5 dataset and passed array do not match in loadHyperslab");
            }
    

            this->loadShape();
        }

        ~Hdf5Array(){
            H5Tclose(datatype_);
            H5Dclose(dataset_);
        }

        uint64_t dimension()const{
            return shape_.size();
        }
        uint64_t shape(const size_t d)const{
            return shape_[d];
        }

        const std::vector<uint64_t> & shape()const{
            return shape_;
        }

        template<class ITER>
        void subarray(
            ITER roiBeginIter,
            marray::View<T> & out
        )const{
            NIFTY_CHECK_OP(out.dimension(),==,this->dimension(),"out has wrong dimension");
            NIFTY_CHECK(out.coordinateOrder() == marray::FirstMajorOrder, 
                "currently only views with last major order are supported"
            );
            this->loadHyperslab(roiBeginIter, roiBeginIter+out.dimension(), out.shapeBegin(), out);
        }


    private:
        template<class BaseIterator, class ShapeIterator>
        void loadHyperslab(
            BaseIterator baseBegin,
            BaseIterator baseEnd,
            ShapeIterator shapeBegin,
            marray::View<T> & out
        ) const {
            HandleCheck<marray::MARRAY_NO_DEBUG> handleCheck;

            // determine shape of hyperslab and array
            std::size_t size = std::distance(baseBegin, baseEnd);
            std::vector<hsize_t> offset(size);
            std::vector<hsize_t> slabShape(size);
            std::vector<hsize_t> marrayShape(size);
            marray::CoordinateOrder coordinateOrder;
            if(H5Aexists(dataset_, reverseShapeAttributeName) > 0) {
                NIFTY_CHECK(false, "currently we do not allow to load from datasets with reverseShapeAttribute")
            } 
            else {
                // don't reverse base and shape
                coordinateOrder = marray::FirstMajorOrder;
                for(std::size_t j=0; j<size; ++j) {
                    offset[j] = hsize_t(*baseBegin);
                    slabShape[j] = hsize_t(*shapeBegin);
                    marrayShape[j] = slabShape[j];
                    ++baseBegin;
                    ++shapeBegin;
                }
            }
            
            hid_t dataspace = H5Dget_space(dataset_);
            herr_t status = H5Sselect_hyperslab(dataspace, H5S_SELECT_SET, 
                &offset[0], NULL, &slabShape[0], NULL);
            if(status < 0) {
                H5Sclose(dataspace);
                throw std::runtime_error("Marray cannot select hyperslab. Check offset and shape!");
            }

            // select memspace hyperslab
            hid_t memspace = H5Screate_simple(int(size), &marrayShape[0], NULL);
            std::vector<hsize_t> offsetOut(size, 0); // no offset
            status = H5Sselect_hyperslab(memspace, H5S_SELECT_SET, &offsetOut[0],
                NULL, &marrayShape[0], NULL);
            if(status < 0) {
                H5Sclose(memspace); 
                H5Sclose(dataspace);
                throw std::runtime_error("Marray cannot select hyperslab. Check offset and shape!");
            }

            // read from dataspace into memspace
            //out = Marray<T>(SkipInitialization, &marrayShape[0], 
            //    (&marrayShape[0])+size, coordinateOrder);
            
            if(out.isSimple()){
                status = H5Dread(dataset_, datatype_, memspace, dataspace,
                    H5P_DEFAULT, &(out(0)));
            }
            else{
                marray::Marray<T> tmpOut(marray::SkipInitialization, &marrayShape[0], 
                    (&marrayShape[0])+size, coordinateOrder);
                out = tmpOut;
            }

            // clean up
            H5Sclose(memspace); 
            H5Sclose(dataspace);
            if(status < 0) {
                throw std::runtime_error("Marray cannot read from dataset.");
            }
            handleCheck.check();
        }


        void loadShape(){

            marray::marray_detail::Assert(marray::MARRAY_NO_ARG_TEST || groupHandle_ >= 0);
            HandleCheck<marray::MARRAY_NO_DEBUG> handleCheck;

            hid_t filespace = H5Dget_space(dataset_);
            hsize_t dimension = H5Sget_simple_extent_ndims(filespace);
            hsize_t* shape = new hsize_t[(std::size_t)(dimension)];
            herr_t status = H5Sget_simple_extent_dims(filespace, shape, NULL);
            if(status < 0) {
                H5Sclose(filespace);
                delete[] shape;
                throw std::runtime_error("Marray cannot get extension of dataset.");
            }
            // write shape to shape_
            shape_.resize(dimension);
            if(H5Aexists(dataset_, reverseShapeAttributeName) > 0) {
                for(std::size_t j=0; j<shape_.size(); ++j) {
                   shape_[shape_.size()-j-1] = uint64_t(shape[j]);
                }
            }
            else {
                for(std::size_t j=0; j<shape_.size(); ++j) {
                    shape_[j] = uint64_t(shape[j]);
                }
            }
            // clean up
            delete[] shape;
            H5Sclose(filespace);
            handleCheck.check();
        }

        hid_t groupHandle_;
        hid_t dataset_;
        hid_t datatype_;
        std::vector<uint64_t> shape_;
    };


} // namespace nifty::graph
} // namespace nifty

#endif  // NIFTY_HDF5_HDF5_ARRAY

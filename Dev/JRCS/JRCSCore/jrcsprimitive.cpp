#include "jrcsprimitive.h"
namespace JRCS{

Plate::Plate():corners_(3,4,arma::fill::zeros),R_(3,3,arma::fill::eye),t_(3,arma::fill::zeros)
{
    t_ = obj_pos_;
}

Plate::Plate(
    const arma::fmat& v,
    const arma::fmat& n,
    const arma::Mat<uint8_t>& c,
    const arma::fvec& pos
):corners_(3,4,arma::fill::zeros),R_(3,3,arma::fill::eye),t_(3,arma::fill::zeros)
{
    xv_.reset(new arma::fmat(v.memptr(),v.n_rows,v.n_cols));
    xn_.reset(new arma::fmat(n.memptr(),v.n_rows,v.n_cols));
    xc_.reset(new arma::Mat<uint8_t>(c.memptr(),c.n_rows,c.n_cols));
//    std::cerr<<"xv("<<xv_->n_rows<<","<<xv_->n_cols<<")"<<std::endl;
    corners_ = *xv_;
//    std::cerr<<"corners("<<corners_.n_rows<<","<<corners_.n_cols<<")"<<std::endl;
    centroid_ = arma::mean(*xv_,1);
//    std::cerr<<"centroid:"<<centroid_.t()<<std::endl;
    size_ = arma::max(arma::abs(corners_.each_col() - centroid_),1);
//    std::cerr<<"size:"<<size_.t()<<std::endl;
    obj_pos_ = pos;
}

void Plate::translate(
        const arma::fvec& t,
        Plate& result
        )
{
    result.t_ = t_ + t;
}

void Plate::transform(
        const arma::fmat& R,
        const arma::fvec& t,
        Plate& result
        )
{
    result.R_ = R*R_;
    result.t_ = R*t_ + t;
}

arma::vec Plate::get_dist2(
        const arma::fmat& v
        )
{
//    std::cerr<<"getting dist a"<<std::endl;
//    std::cerr<<"v:"<<v.n_rows<<","<<v.n_cols<<std::endl;
//    std::cerr<<t_<<std::endl;
    //transform back to axis aligned space
    arma::fmat tv = v.each_col() - t_;
//    std::cerr<<"getting dist b"<<std::endl;
    arma::fmat invR = R_.i();
    assert(invR.is_finite());
    tv = R_.i()*tv;
    std::cerr<<"__"<<std::endl;
    return dist(tv,0)+dist(tv,1)+dist(tv,2);
}


arma::vec Plate::dist(const arma::fmat& v, arma::uword dim)
{
    arma::vec result(v.n_cols,arma::fill::zeros);

    arma::frowvec vdim = v.row(dim);
    arma::uvec idx_dim = arma::find( arma::abs( vdim - centroid_(dim) ) > size_(dim));
    result(idx_dim) = arma::square(arma::abs( arma::conv_to<arma::vec>::from(vdim.cols(idx_dim)) - centroid_(dim) ) - size_(dim));

    return result;
}

void Plate::get_weighted_centroid(
        const arma::fmat& v,
        const arma::vec& alpha
        )
{
    ;
}

void Plate::accumulate(
        const arma::fmat& v,
        const arma::fmat& n,
        const arma::Mat<uint8_t>& c,
        const arma::vec alpha
        )
{
    ;
}

void Plate::accumulate(const Plate&)
{
    ;
}

void Plate::average(void)
{
    ;
}

JRCSPrimitive::JRCSPrimitive():JRCSBilateral(),plate_num_for_obj_(5),point_num_for_plate_(4)
{

}

void JRCSPrimitive::initx(
        const MatPtr& xv,
        const MatPtr& xn,
        const CMatPtr& xc
        )
{
    if(verbose_)std::cerr<<"SJRCSBase start init x"<<std::endl;

    xv_ptr_ = xv;
    xn_ptr_ = xn;
    xc_ptr_ = xc;

    max_obj_radius_ = 1.0;// for randomly reset rt

    xv_ptr_->fill(std::numeric_limits<float>::quiet_NaN());
    xn_ptr_->fill(std::numeric_limits<float>::quiet_NaN());
    xc_ptr_->fill(0);

    int k = xv_ptr_->n_cols;
    if(verbose_){
        std::cerr<<"k:"<<k<<"=="<<xn_ptr_->n_cols<<"=="<<xc_ptr_->n_cols<<std::endl;
    }

    int r_k = k;
    float* pxv = (float*)xv_ptr_->memptr();
    float* pxn = (float*)xn_ptr_->memptr();
    uint8_t* pxc = (uint8_t*)xc_ptr_->memptr();
    int N = obj_prob_.size();
    obj_pos_ = arma::fmat(3,N,arma::fill::zeros);
    arma::frowvec z = arma::linspace<arma::frowvec>(float(-N/2),float(N/2),N);
    obj_pos_.row(2) = z;
    if(verbose_>0){
        std::cerr<<"obj_pos_:"<<std::endl;
        std::cerr<<obj_pos_<<std::endl;
    }
    obj_range_.resize(2*N);
    plate_ptrlst_.resize(plate_num_for_obj_*obj_num_);
    arma::uword* pxr = (arma::uword*)obj_range_.data();
    arma::uword* pxr_s = pxr;
    *pxr = 0;
    for(int obj_idx = 0 ; obj_idx < obj_num_ ; ++ obj_idx )
    {
        int obj_size = plate_num_for_obj_*point_num_for_plate_;
        if( pxr > pxr_s )*pxr = (*(pxr - 1))+1;
        *(pxr+1) = *pxr + plate_num_for_obj_ - 1;

        arma::fmat objv(pxv,3,obj_size,false,true);
        arma::fmat objn(pxn,3,obj_size,false,true);
        arma::Mat<uint8_t> objc(pxc,3,obj_size,false,true);

        arma::fvec pos = obj_pos_.col(obj_idx);
        reset_obj_vn(0.5,pos,objv,objn);
        reset_obj_c(objc);

        for(int j=0 ; j < plate_num_for_obj_; ++j)
        {
            plate_ptrlst_[obj_idx*plate_num_for_obj_+j].reset(
                new Plate(
                    arma::fmat(pxv,3,point_num_for_plate_,false,true),
                    arma::fmat(pxn,3,point_num_for_plate_,false,true),
                    arma::Mat<uint8_t>(pxc,3,point_num_for_plate_,false,true),
                    obj_pos_.col(obj_idx)
                )
            );
            pxv += 3*point_num_for_plate_;
            pxn += 3*point_num_for_plate_;
            pxc += 3*point_num_for_plate_;
        }

        pxr += 2;
        r_k -= obj_size;
    }
    if(verbose_)std::cerr<<"Done initx_"<<std::endl;
}

void JRCSPrimitive::reset_obj_vn(
        float radius,
        arma::fvec& pos,
        arma::fmat& ov,
        arma::fmat& on
        )
{
    arma::fmat v = {
        {-1, 1, 1,-1, 1,-1,-1, 1, 1, 1, 1, 1,-1,-1,-1,-1,-1,-1, 1, 1},
        { 1, 1, 1, 1,-1,-1,-1,-1, 1,-1,-1, 1,-1, 1, 1,-1, 1,-1,-1, 1},
        { 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1}
    };
    ov = v;
    ov *= radius;
    ov.each_col() += pos;
    arma::fmat n = {
        { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1,-1,-1,-1,-1, 0, 0, 0, 0},
        { 1, 1, 1, 1,-1,-1,-1,-1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1}
    };
    on = n;
}
void JRCSPrimitive::compute(void)
{
    if(verbose_)std::cerr<<"preparing"<<std::endl;
    prepare_primitive();
    while(!isEnd_primitive())
    {
        if(verbose_)std::cerr<<"step 1"<<std::endl;
        #pragma omp parallel for
        for( int i=0 ; i < vvs_ptrlst_.size() ; ++i )
        {
            step_1(i);
        }
        if(verbose_)std::cerr<<"step 2"<<std::endl;
        step_2();
        finish_primitive();
    }
//    JRCSBilateral::compute();
}

void JRCSPrimitive::prepare_primitive()
{
    iter_count_ = 0;
    reset_alpha_primitive();
    plate_t_ptrlst_.resize(vvs_ptrlst_.size());
    #pragma omp parallel for
    for( int i = 0 ; i < vvs_ptrlst_.size() ; ++i )
    {
        plate_t_ptrlst_[i].resize(plate_ptrlst_.size());
        for(int j=0 ; j < plate_t_ptrlst_[i].size() ; ++j )
        {
            plate_t_ptrlst_[i][j].reset(new Plate());
        }
    }
    reset_prob_primitive();
}

void JRCSPrimitive::finish_primitive()
{
    ++iter_count_;
}

void JRCSPrimitive::step_1(int i)
{
    //input
    arma::fmat& vv_ = *vvs_ptrlst_[i];
    arma::fmat& vn_ = *vns_ptrlst_[i];
    arma::Mat<uint8_t>& vc_ = *vcs_ptrlst_[i];
    if(verbose_>1)std::cerr<<"transform latent model"<<std::endl;
    Ts& rt = rt_lst_[i];
    for(int o = 0 ; o < obj_num_ ; ++o )
    {
        arma::fmat R(rt[o].R,3,3,false,true);
        arma::fvec t(rt[o].t,3,false,true);
        for(int j=obj_range_[2*o];j<=obj_range_[2*o+1];++j)
        {
            plate_ptrlst_[j]->transform(R,t,*plate_t_ptrlst_[i][j]);
        }
    }

    if(verbose_>1)std::cerr<<"calculate alpha"<<std::endl;
    arma::mat&  alpha = *alpha_ptrlst_[i];
    for(int c = 0 ; c < alpha.n_cols ; ++c )
    {
        alpha.col(c) = plate_t_ptrlst_[i][c]->get_dist2(vv_);
    }

    alpha = arma::trunc_exp(alpha);
    alpha.each_row() %=  arma::pow(xv_invvar_,1.5);

    if(verbose_>1)std::cerr<<"normalize alpha"<<std::endl;
    alpha += std::numeric_limits<double>::epsilon(); //add eps for numeric stability
    arma::vec alpha_rowsum = ( 1.0 + beta_ ) * arma::sum(alpha,1);
    alpha.each_col() /= alpha_rowsum;

    if(!alpha.is_finite())
    {
        std::cerr<<iter_count_<<":invalid alpha in step_a("<<i<<")"<<std::endl;
    }

    //update RT
    if(verbose_>1)std::cerr<<"#1 calculate weighted plate centers"<<std::endl;
    arma::frowvec alpha_colsum = arma::conv_to<arma::frowvec>::from(arma::sum( alpha ));
    for(int c = 0 ; c < alpha.n_cols ; ++c )
    {
        for( int p = 0 ; p < plate_t_ptrlst_[i].size() ; ++ p )
        {
            plate_t_ptrlst_[i][p]->get_weighted_centroid(vv_,alpha.col(c));
        }
    }

    if(verbose_>1)std::cerr<<"#2 calculating RT for each object"<<std::endl;
    for(int o = 0 ; o < obj_num_ ; ++o )
    {
        arma::fmat A;
        arma::fmat U,V;
        arma::fvec s;
        arma::fmat dR;
        arma::fvec dt;
        arma::fmat R(rt[o].R,3,3,false,true);
        arma::fvec t(rt[o].t,3,false,true);
        arma::fmat _v(3,plate_num_for_obj_);
        arma::fmat objv(3,plate_num_for_obj_);
        int vi = 0;
        for(int p=obj_range_[2*o];p<=obj_range_[2*o+1];++p)
        {
            _v.col(vi) = plate_t_ptrlst_[i][p]->weighted_centroid_;
            objv.col(vi) = plate_t_ptrlst_[i][p]->centroid_;
            ++vi;
        }
        arma::fmat cv = _v.each_col() - arma::mean(_v,1);
        if(!_v.is_finite())
        {
            std::cerr<<iter_count_<<":"<<o<<":!v.is_finite()"<<std::endl;
        }

        arma::frowvec square_lambdav = arma::conv_to<arma::frowvec>::from(xv_invvar_.cols(obj_range_[2*o],obj_range_[2*o+1]));
        square_lambdav %= alpha_colsum.cols(obj_range_[2*o],obj_range_[2*o+1]);
        square_lambdav += std::numeric_limits<float>::epsilon(); // for numeric stability
        if(!square_lambdav.is_finite())
        {
            std::cerr<<"!square_lambda.is_finite()"<<std::endl;
        }
        arma::frowvec pv(square_lambdav.n_cols,arma::fill::ones);
        arma::frowvec square_norm_lambdav = square_lambdav / arma::accu(square_lambdav);
        if(!square_norm_lambdav.is_finite())
        {
            std::cerr<<"!square_norm_lambda.is_finite()"<<std::endl;
        }
        pv -= square_norm_lambdav;
        arma::fmat tmpv = objv.each_col() - arma::mean(objv,1) ;
        tmpv.each_row() %= pv % square_lambdav;

        A = cv*tmpv.t();

        switch(rttype_)
        {
        case Gamma:
        {
            arma::fmat B = A.submat(0,0,1,1);
            dR = arma::fmat(3,3,arma::fill::eye);
            if(arma::svd(U,s,V,B,"std"))
            {
                arma::fmat C(2,2,arma::fill::eye);
                C(1,1) = arma::det( U * V.t() )>=0 ? 1.0 : -1.0;
                arma::fmat dR2D = U*C*(V.t());
                dR.submat(0,0,1,1) = dR2D;
                if(!dR.is_finite())
                {
                    std::cerr<<iter_count_<<":!dR.is_finite()"<<std::endl;
                    dR.fill(arma::fill::eye);
                }
                arma::fmat tmp = _v - dR*objv;
                tmp.each_row() %= square_norm_lambdav;
                dt = arma::sum(tmp,1);
                if(!dt.is_finite())
                {
                    std::cerr<<iter_count_<<":!dt.is_finite()"<<std::endl;
                    dt.fill(arma::fill::zeros);
                }
            }
        }
            break;
        default:
        {
            if(arma::svd(U,s,V,A,"std"))
            {
                arma::fmat C(3,3,arma::fill::eye);
                C(2,2) = arma::det( U * V.t() )>=0 ? 1.0 : -1.0;
                dR = U*C*(V.t());
                if(!dR.is_finite())
                {
                    std::cerr<<iter_count_<<":!dR.is_finite()"<<std::endl;
                    dR.fill(arma::fill::eye);
                }
                arma::fmat tmp = _v - dR*objv;
                tmp.each_row() %= square_norm_lambdav;
                dt = arma::sum(tmp,1);
                if(!dt.is_finite())
                {
                    std::cerr<<iter_count_<<":!dt.is_finite()"<<std::endl;
                    dt.fill(arma::fill::zeros);
                }
            }
        }
        }

        //transforming transformed object
        for(int p=obj_range_[2*o];p<=obj_range_[2*o+1];++p)
        {
            plate_t_ptrlst_[i][p]->transform(dR,dt,*plate_t_ptrlst_[i][p]);
        }

        //updating R T
        R = dR*R;
        t = dR*t + dt;
    }
    if(verbose_>1)std::cerr<<"#3 done RT for each object"<<std::endl;

    arma::mat alpha_v2(alpha.n_rows,alpha.n_cols);
    for(int c=0;c<alpha_v2.n_cols;++c)
    {
        alpha_v2.col(c) = plate_t_ptrlst_[i][c]->get_dist2(vv_);
    }
    vvar_.row(i) = arma::sum(alpha_v2%alpha);

    if(verbose_>1)std::cerr<<"#3 done RT for each object"<<std::endl;
    for(int c=0;c<alpha.n_rows;++c)
    {
        plate_t_ptrlst_[i][c]->accumulate(vv_,vn_,vc_,alpha.col(c));
    }
}

void JRCSPrimitive::step_2(void)
{
    if(verbose_>1)std::cerr<<"Updating Latent Model"<<std::endl;
    #pragma omp parallel for
    for( int i=0 ; i < plate_ptrlst_.size() ; ++i )
    {
        for(int j=0;j<plate_t_ptrlst_.size();++j)
        {
            plate_ptrlst_[i]->accumulate(*plate_t_ptrlst_[j][i]);
        }
        plate_ptrlst_[i]->average();
    }

    if(verbose_>1)std::cerr<<"Updating variance of Latent Model"<<std::endl;
    arma::rowvec alpha_sum;
    for(int i=0;i<alpha_ptrlst_.size();++i)
    {
        arma::mat& alpha = *alpha_ptrlst_[i];
        if(alpha_sum.empty())alpha_sum = arma::sum(alpha);
        else alpha_sum += arma::sum(alpha);
    }

    if(verbose_>1)std::cerr<<"Restore reciprocal fractions of variation"<<std::endl;
    xv_invvar_ = ( ( 3.0*alpha_sum ) / ( arma::sum(vvar_) + beta_ ) );

    if(verbose_>1)std::cerr<<"Updating weight of centroids"<<std::endl;
    float mu = arma::accu(alpha_sum);
    mu *= ( 1.0 + beta_ );
    x_p_ = alpha_sum;
    if( mu != 0)x_p_ /= mu;
}

bool JRCSPrimitive::isEnd_primitive(void)
{
    if(iter_count_>=1)return true;
    else return false;
}

void JRCSPrimitive::reset_alpha_primitive()
{
    if(verbose_>0)std::cerr<<"allocating alpha"<<std::endl;
    int N = plate_ptrlst_.size();
    int idx=0;
    while( idx < vvs_ptrlst_.size() )
    {
        if(idx>=alpha_ptrlst_.size())alpha_ptrlst_.emplace_back(new arma::mat(vvs_ptrlst_[idx]->n_cols,N));
        else if((alpha_ptrlst_[idx]->n_rows!=vvs_ptrlst_[idx]->n_cols)||(alpha_ptrlst_[idx]->n_cols!=N))
            alpha_ptrlst_[idx].reset(new arma::mat(vvs_ptrlst_[idx]->n_cols,N));
        ++idx;
    }
    if(verbose_>0)std::cerr<<"done allocating alpha"<<std::endl;
}

void JRCSPrimitive::reset_prob_primitive()
{
    if(verbose_>0)std::cerr<<"initializing probability"<<std::endl;

    arma::fvec maxAllXYZ,minAllXYZ;
    MatPtrLst::iterator vviter;
    for( vviter = vvs_ptrlst_.begin() ; vviter != vvs_ptrlst_.end() ; ++vviter )
    {
        arma::fmat&v = **vviter;
        arma::fvec maxVXYZ = arma::max(v,1);
        arma::fvec minVXYZ = arma::min(v,1);
        if(vviter==vvs_ptrlst_.begin())
        {
            maxAllXYZ = maxVXYZ;
            minAllXYZ = minVXYZ;
        }else{
            maxAllXYZ = arma::max(maxVXYZ,maxAllXYZ);
            minAllXYZ = arma::min(minVXYZ,minAllXYZ);
        }
    }

    float r = maxAllXYZ(2)-minAllXYZ(2);
    float maxvar = r*r;
    xv_invvar_ = arma::rowvec(plate_ptrlst_.size());
    xv_invvar_.fill(1.0/maxvar);

    vvar_ = arma::mat(vvs_ptrlst_.size(),plate_ptrlst_.size());

    x_p_ = arma::rowvec(plate_ptrlst_.size());
    x_p_.fill(1.0/float(plate_ptrlst_.size()));

    if(verbose_>0)std::cerr<<"done probability"<<std::endl;
}

}
#ifndef NSBO_HPP_
#define NSBO_HPP_

#include <algorithm>
#include "bo_multi.hpp"


namespace limbo {


  template <
    class Params
    , class A2 = boost::parameter::void_
    , class A3 = boost::parameter::void_
    , class A4 = boost::parameter::void_
    , class A5 = boost::parameter::void_
    , class A6 = boost::parameter::void_
    , class A7 = boost::parameter::void_
    >
  class Nsbo : public BoMulti<Params, A2, A3, A4, A5, A6, A7> {
   public:
    typedef std::tuple<Eigen::VectorXd, Eigen::VectorXd, Eigen::VectorXd> pareto_point_t;

    template<typename EvalFunction>
    void optimize(const EvalFunction& feval, bool reset = true) {
      this->_init(feval, reset);

      while (this->_samples.size() == 0 || this->_pursue(*this)) {
        std::cout << "updating pareto model...";
        std::cout.flush();
        this->template update_pareto_model<EvalFunction::dim>();
        std::cout << "ok" << std::endl;
        auto pareto = this->pareto_model();

        // Pareto front of the variances
        auto p_variance = pareto::pareto_set<2>(pareto);
        auto best = p_variance[rand() % p_variance.size()];
        Eigen::VectorXd best_v = std::get<0>(best);

        this->add_new_sample(best_v, feval(best_v));
        this->_iteration++;
        std::cout << this->_iteration << " | " << best_v.transpose()
                  << "-> " << this->_observations.back().transpose()
                  << " (expected:" << this->_models[0].mu(best_v) << " "
                  << this->_models[1].mu(best_v) << ")"
                  << " sigma:" << this->_models[0].sigma(best_v)
                  << " " << this->_models[1].sigma(best_v)
                  << std::endl;
        this->_update_stats(*this); // with the *this, nolonger need to redeclare the _update_stats() for each class.
      }

    }

   protected:
    // former way to deal with the template of RefreshStat
    /*    void _update_stats() {
      boost::fusion::for_each(this->_stat, RefreshStat_f<Nsbo>(*this));
      }*/

  };

}

#endif

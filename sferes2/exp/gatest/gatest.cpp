#include <iostream>
#include <sferes/phen/parameters.hpp>
#include <sferes/gen/evo_float.hpp>
#include <sferes/ea/nsga2.hpp>
#include <sferes/eval/eval.hpp>
#include <sferes/stat/pareto_front.hpp>
#include <sferes/stat/best_fit.hpp>
#include <sferes/stat/mean_fit.hpp>
#include <sferes/modif/dummy.hpp>
#include <sferes/run.hpp>
#include <boost/program_options.hpp>
#include <boost/foreach.hpp>
#include <boost/assign/list_of.hpp>
#include "boost/random.hpp"
#include "boost/generator_iterator.hpp"

#include "simulation.hh"

#include "map_elites.hpp"
#include "fit_map.hpp"
#include "stat_map.hpp"

using namespace sferes;
using namespace sferes::gen::evo_float;

bool headless = true;
boost::shared_ptr<robot::robot4> orob;
boost::shared_ptr<ode::Environment> oenv;
/*
   struct Params {
   struct evo_float {
   SFERES_CONST float cross_rate = 0.2f;
   SFERES_CONST float mutation_rate = 0.1f;
   SFERES_CONST float eta_m = 15.0f;
   SFERES_CONST float eta_c = 10.0f;
   SFERES_CONST mutation_t mutation_type = polynomial;
   SFERES_CONST cross_over_t cross_over_type = sbx;
   };
   struct pop {
   SFERES_CONST unsigned size = 300;
   SFERES_CONST unsigned nb_gen = 500;
   SFERES_CONST int dump_period = 5;
   SFERES_CONST int initial_aleat = 1;
//SFERES_CONST float coeff = 1.1f;
//SFERES_CONST float keep_rate = 0.6f;
};
struct parameters {
SFERES_CONST float min = 0.0f;
SFERES_CONST float max = 1.0f;
};
};*/

struct Params {
    struct ea {
        SFERES_CONST size_t behav_dim = 2;
        SFERES_CONST double epsilon = 0;//0.05;
        SFERES_ARRAY(size_t, behav_shape, 128, 128);
    };
    struct pop {
        // number of initial random points
        SFERES_CONST size_t init_size = 300;
        // size of a batch
        SFERES_CONST size_t size = 300;
        SFERES_CONST size_t nb_gen = 400;
        SFERES_CONST size_t dump_period = 5;
    };
    struct parameters {
        SFERES_CONST float min = 0.0f;
        SFERES_CONST float max = 1.0f;
    };
    struct evo_float {
        SFERES_CONST float cross_rate = 0.25f;
        SFERES_CONST float mutation_rate = 0.1f;
        SFERES_CONST float eta_m = 15.0f;
        SFERES_CONST float eta_c = 10.0f;
        SFERES_CONST mutation_t mutation_type = polynomial;
        SFERES_CONST cross_over_t cross_over_type = sbx;
    };
};

FIT_MAP(FitZDT2){
    public :
        FitZDT2()  {}
        template<typename Indiv>
            void eval(Indiv& ind) {
                //std::unique_ptr<Simulation> sim;
                //this->_objs.resize(1);
                //sim.reset(new Simulation(0.00f, 200, 6, headless));
                Simulation sim(orob, 0.00f, 10, 6, headless);
                float result = sim.run(ind, 0.008f, 4);
                this->_value = result;

                std::vector<float> data;
                data.push_back((ind.gen().data(6)+ind.gen().data(12))/2);//amplitudes of joints that lift
                data.push_back((ind.gen().data(9)+ind.gen().data(15))/2);//amplitudes of joints that sweep
                //std::cout << (ind.gen().data(6)+ind.gen().data(12))/2 << std::endl;
                //std::cout << (ind.gen().data(9)+ind.gen().data(15))/2 << std::endl;
                //std::cout << "---------" << std::endl;

                this->set_desc(data);
                //this->_objs[0] = result;
            }
            bool dead(){
                return false;
            }
};

int main(int argc, char **argv) {
    std::cout<<"running "<<argv[0]<<" ... try --help for options (verbose)"<<std::endl;
    //headless = atoi(argv[1]);
    dInitODE2(0);
    oenv = boost::shared_ptr<ode::Environment>(new ode::Environment(0.0f, 0.0f, 0.0f));
    orob = boost::shared_ptr<robot::robot4>(new robot::robot4(*oenv, Eigen::Vector3d(0, 0, 0.5)));
    typedef gen::EvoFloat<18, Params> gen_t;
//    typedef gen::Sampled<18, Params> gen_t;
    typedef phen::Parameters<gen_t, FitZDT2<Params>, Params> phen_t;
    typedef eval::Parallel<Params> eval_t;
    typedef boost::fusion::vector<stat::Map<phen_t, Params>, stat::BestFit<phen_t, Params> > stat_t;
    typedef modif::Dummy<> modifier_t;
    typedef ea::MapElites<phen_t, eval_t, stat_t, modifier_t, Params> ea_t;
    ea_t ea;


    run_ea(argc, argv, ea);
  float best = ea.stat<1>().best()->fit().value();
  std::cout<<"best fit (map_elites):" << best << std::endl;
    //  std::cout<<"==> mean fitness ="<<ea.stat<1>().mean()<<std::endl;
    dCloseODE();
    return 0;
}


#include <iostream>

#include <boost/foreach.hpp>
#include <boost/assign/list_of.hpp>
#include "boost/random.hpp"
#include "boost/generator_iterator.hpp"

#include "simulation.hh"

Simulation::Simulation(const robot_t& orob, const float tilt, const int count,
        const int size, const bool headless) : env(new ode::Environment(0.0f, tilt, 0.0f)){
    this->headless = headless;
    this->tilt = tilt;

    rob = orob->clone(*env); //clone returns boost

    if(!headless){
        this->v.reset(new renderer::OsgVisitor()); //assures that v is updated
        rob->accept(*v);
    }

    add_blocks(count, size);
}

/* Uses a 2D gaussian to spread blocks on the surface
 * https://en.wikipedia.org/wiki/Gaussian_function#Two-dimensional_Gaussian_function
 */
void Simulation::add_blocks(int count, int size){

    float xc = -0.4; //skew gauss and location
    float yc = 0;
    float s = 0.5; //spread gauss and location

    typedef boost::mt19937 RNGType;
    RNGType rng( time(0) );
    /* s-1 is the location based on how spread it is, which wraps the gaussian bell over
     * the relevant parts making the gauss and the locations in the same range.
     * The reason we subtract 1 is to chop off the "skirts" of the gauss, prevent the creation
     * of miniscule boxes which does nothing but impact performance.
     */
    boost::uniform_real<> loc_range(-(s-0.1),s-0.1);
    boost::variate_generator<boost::mt19937&, boost::uniform_real<> > rlocation(rng,loc_range);
    boost::uniform_real<> size_range(0.002, (float) size/1000);
    boost::variate_generator<boost::mt19937&, boost::uniform_real<> > rsize(rng,size_range);

    for(int i = 0; i < count; ++i){
        //Gaussian gaussian amplitudes
        float a = rsize();

        float x = rlocation() + xc; //random + gaussian skew
        float y = rlocation() + yc;

        //2D gaussian
        float bsize = a*exp( -( (pow((x-xc), 2) / (2*pow(s, 2)) ) +
                    (pow((y-yc), 2) / (2*pow(s, 2)) ) ));


        /*tan(angle) is conversion from degrees to slope (relationship between rise and run)
         *Multiplying this with -x will find the correct height of the block based on the
         *rise. This means that you can place boxes along x and y in a slope, and the boxes will
         *follow the slope
         */
        ode::Object::ptr_t b
            (new ode::Box(*env, Eigen::Vector3d(x, y, bsize/2 + (tan(tilt)*-x)),
                          10, bsize*4, bsize*4, bsize)); //multiply by 4 to stretch out

        b->set_rotation(0.0f, -tilt, 0.0f);
        boxes.push_back(b);
        if(!headless){
            b->accept(*v);
        }
        b->fix();
        env->add_to_ground(*b);
    }
}

float Simulation::run_conf(std::vector<float> config, const float step, const int step_limit){

    while(x < step_limit) {
        if(!headless){
            if(v->done()){ //If user presses escape in window
                exit(EXIT_SUCCESS); //abort everything including sferes-backend
            }
        }
        procedure(config, step);
    }

    Eigen::Vector3d pos = rob->pos();
    //std::cout << "Fitness: " << -pos(0) << std::endl;
    return -pos(0);
}

void Simulation::procedure(std::vector<float> data, const float step){
    x += step;
    //rob.bodies()[1]->fix();
    if(!headless) {
        v->update();
    }
    rob->next_step(step);
    env->next_step(step);
    int genptr = 0;
    for (size_t i = 0; i < rob->servos().size() - 4; ++i){
        float a = data.at(genptr++) * 40.0f;
        float theta = data.at(genptr++) * 1.0f;
        float b = (data.at(genptr++) * 40.0f) - 20.0f;
        //std::cout << "Servo(" << i << "): " << a << " " << theta << " " << b << std::endl;
        float h = 4;
        float f = data.back() * 2.0f; //last element adjusts frequency to a max of 2

        if(a < 5.0f){ //If amplitude is below threshold
            a = 0.0f; //set to zero
        }


        double phase = a*tanh(h*sin((f*M_PI)*(x+theta))) + b;

        if(i == 6 || i == 9){ //if outer joints
            phase = phase * 1.8f; //amplify phase to prevent stunted mobility of robot4
        }


        if(i <= 1){ //body joints only
            rob->servos()[i]->set_angle(ode::Servo::DIHEDRAL, phase * M_PI/180);
        }else{
            rob->servos()[i]->set_angle(ode::Servo::DIHEDRAL, phase * M_PI/180);
            rob->servos()[i+4]->set_angle(ode::Servo::DIHEDRAL, phase * M_PI/180); //and opposite for other side
        }
    }
}

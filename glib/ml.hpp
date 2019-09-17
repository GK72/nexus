// **********************************************
// ** gkpro @ 2019-08-26                       **
// **                                          **
// **           ---  G-Library  ---            **
// **        Machine Learning Library          **
// **          Class: Neural network           **
// **********************************************

#pragma once

#include <ctime>
#include <random>
#include <string>
#include <vector>

#include "gmath.h"

namespace glib {
namespace ml {

using glib::gint;
using matrix = glib::Matrix<double>;
template <class T> using vec = std::vector<T>;

// ************************************************************************** //
//                                Functions                                   //
// ************************************************************************** //

// Cost function for linear regression
double costLiR(double x, double y) {
    return std::pow(x - y, 2);
}

// Cost function for logistic regression
double costLoR(double x, double y) {
    return -y * std::log10(x) - (1 - y) * std::log10(1 - x);
}

double avgCost(matrix& x, matrix& y, double (*costfunc)(double, double)) {
    gint length = x.getN();                     // # training data
    double sum = 0;
    for (size_t i = 0; i < length; ++i) {
        sum += costfunc(x[i], y[i]);
    }
    return sum / length / 2;
}

// Partial derivative
double dee(double x) {
    return 1;
}
double gradientDesc(matrix& x, matrix& y, matrix& theta, double alpha) {
    gint length = x.getN();
    matrix temp(theta);
    for (size_t i = 0; i < length; ++i)	{
        // TODO: complete partial derivative function (dee)
        // If logistic regression, run sigmoid function before calculating cost
        temp[i] = theta[i] - alpha * dee(avgCost(x, y, costLoR));
    }
	
    return 0;
}

// ************************************************************************** //
//                              Neural Network                                //
// ************************************************************************** //

struct nnConfig {
    std::vector<gint> layerSizes;
    // Input layer  : # features
    // Output layer : # classes

};

// Input layer = features

// Neural network
class NN {
public:
    NN(nnConfig cfg) {
        L = cfg.layerSizes.size();
        config = cfg;
        for (size_t i = 0; i < L; ++i) {
            a.push_back(new matrix(cfg.layerSizes[i]));
        }
        // For consistent layer indexing
        // Input layer does not have weights or biases
        w.push_back(nullptr);
        b.push_back(nullptr);
        delta.push_back(nullptr);
        y = new matrix;

        for (size_t i = 1; i < L; ++i) {
            // ith element in a row is the weigth for the ith node in the next layer
            // rows for nodes in the next layer
            // column = weight from node from the prev layer
            // row    = weight to node
            w.push_back(new matrix(cfg.layerSizes[i], cfg.layerSizes[i-1]));
            b.push_back(new matrix(cfg.layerSizes[i]));
            delta.push_back(new matrix(cfg.layerSizes[i]));
            dee_delta.push_back(new matrix(cfg.layerSizes[i]));
        }
    }
    ~NN() {
        for (auto& e : a) delete e;
        for (auto& e : b) delete e;
        for (auto& e : w) delete e;
        for (auto& e : delta) delete e;
        for (auto& e : dee_delta) delete e;
    }

    // Initializing nodes, weights and biases
    void initNet() {
        std::random_device rd{};
        std::mt19937 rng(rd());
        std::uniform_real_distribution<> d(0, 1);
        //std::fisher_f_distribution<> d(0.2);
        //std::cout << d.min() << ' ' << d.max() << '\n';

        for (auto& e : *a[0])               { e = 0; }
        for (size_t i = 1; i < L; ++i) {
            for (auto& e : *a[i])           { e = d(rng); }
            for (auto& e : *b[i])           { e = d(rng); }
            for (auto& e : *delta[i])       { e = d(rng); }
        }
        for (size_t i = 1; i < L; ++i) {
            gint length = config.layerSizes[i] * config.layerSizes[i-1];
            for (size_t j = 0; j < length; ++j) {
                (*w[i])[j] = 0;
            }
        }
        
        // TODO: initialize dee_deltas

        initialized = true;
    }

    void loadInput(const matrix& in_x, const matrix& in_y) {
        a[0]->set(in_x);
        y->set(in_y);
    }

    double cost(const matrix& a, const double &y) {
        matrix c = epow(a - y, 2);
        double sum = 0;
        for (auto& e : c) {
            sum += e;
        }
        return sum /= c.getLength();

        // a[L-1] * deri(sigmoid(z[L])) * 2 * (a[L] - y)
        // sigmoid' = sigmoid(x) * (1 - sigmoid(x))
    }

    double costOpt() {
        
        //costLoR();
    }

    // Forward propagation
    void fprop() {
        // For every layer
        for (size_t i = 1; i < L; ++i) {
            matrix z = *w[i] * *a[i-1] + *b[i];
            *a[i] = sigmoid(z);
        }
    }

    // Backpropagation
    void bprop() {
        *delta[L-1] = *a[L-1] - *y;
        for (size_t i = L - 2; i > 0; --i) {
            // weights * errors, element multiplied by the derivative of activation values
            *delta[i] = edot(w[i+1]->tr() * *delta[i+1], edot(*a[i], 1.0 - *a[i]));
        }
    }

    void partialCostDeltas() {
        for (size_t i = 0; i < L - 1; ++i) {
            *dee_delta[i] = *dee_delta[i] + *delta[i+1] * a[i]->tr();
        }
    }


    // Getter methods
    
    std::string getNumLayers() {
        std::string str;
        for (size_t i = 0; i < a.size(); ++i) {
            str += std::to_string(a[i]->getLength()) + ' ';
        }
        return str;
    }

    std::string getNetState() {
        // TODO (-): Proper tabulated formatting + weights and biases also
        std::string str;
        gint n = config.layerSizes.size();
        gint m = 0;
        gint length;
        for (size_t i = 1; i < n; ++i) {
            if (config.layerSizes[i] > m) {
                m = config.layerSizes[i];
            }
        }

        for (size_t j = 0; j < m; ++j) {
            str += "Neuron " + std::to_string(j + 1) + ": ";
            for (size_t i = 1; i < n; ++i) {
                length = a[i]->getLength();
                if (j < length) {
                    str += std::to_string((*a[i])[j]) + "\t";
                }
            }
            str += '\n';
        }

        return str;
    }

    std::string getActivationValues(gint layer) {
        std::string str;
        gint length = a[layer]->getLength();
        for (size_t i = 0; i < length; ++i)    {
            str += std::to_string((*a[layer])[i]) + ' ';
        }
        return str;
    }

private:
    vec<matrix*> a;                 // Neurons: activation values
    // TODO: (transposed) biases can be augmented to the zeroth element of weights
    vec<matrix*> b;                 // Biases
    vec<matrix*> w;                 // Weights
    vec<matrix*> delta;             // Error
    vec<matrix*> dee_delta;         // Partial derivatives of error
    matrix* y;                      // Expected values
    
    nnConfig config;                // Network configuration
    gint L;                         // Number of layers in the network
    bool initialized = false;
};



} // End of namespace ml
} // End of namespace glib


// Training and test data
// http://yann.lecun.com/exdb/mnist/
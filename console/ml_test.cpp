// **********************************************
// ** gkpro @ 2019-09-05                       **
// **                                          **
// **        ML testing application            **
// **       --- G-Library testing ---          **
// **                                          **
// **********************************************

#pragma once

#include <iostream>
#include <vector>

#include "gmath.h"
#include "ml.hpp"
#include "utility.h"

using vec = std::vector<int>;

void ml_test()
{
    try {
        glib::Matrix<double> y;
        glib::Matrix<double> x;
        y.importIDX(R"#(D:\OneDrive\Developing\data\ml\numbers\t10k-labels.idx1-ubyte)#");
        x.importIDX(R"#(D:\OneDrive\Developing\data\ml\numbers\train-images.idx3-ubyte)#");

        //x = x.tr();

        glib::Matrix<double> x2 = x.getRow(0).tr();
        glib::Matrix<double> y2 = y.getRow(0);
        glib::ml::nnConfig nc;
        nc.layerSizes = std::vector<size_t>{x2.getN(), 5, 5, 10};

        glib::ml::NN nn(nc);
        nn.initNet();
        nn.loadInput(x2, y2);


        //x.exportTXT(R"#(D:\OneDrive\Developing\data\ml\numbers\train-images.txt)#");

    }
    catch (const std::exception& e) {
        std::cout << e.what();
    }
}
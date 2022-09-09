#pragma once
#include "ProjTest.h"
#include "TargaImage.h"
#include "ScriptHandler.h"
#include <vector>
#include <string>
#include <iostream>

void ProjTest::Test() {
    std::vector<std::string> cmds = std::vector<std::string>({
                                                "gray",
                                                "quant-unif",
                                                "quant-pop",
                                                "dither-thresh",
                                                "dither-rand",
                                                "dither-fs",
                                                "dither-bright",
                                                "dither-cluster",
                                                "dither-pattern",
                                                "dither-color",
                                                "filter-box",
                                                "filter-bartlett",
                                                "filter-gauss",
                                                "filter-gauss-n",//
                                                "filter-edge",
                                                "filter-enhance",
                                                "npr-paint",
                                                "half",
                                                "double",
                                                "scale",//
                                                "comp-over",//
                                                "comp-in",//
                                                "comp-out",//
                                                "comp-atop",//
                                                "comp-xor",//
                                                "diff",//
                                                "rotate"//
    });

    TargaImage* pImage = NULL;
    TargaImage* targetImage = NULL;
    for (int i = 0; i < 27; i++) {
        CScriptHandler::HandleCommand("load wiz.tga", pImage);
        std::string cmd = cmds[i];
        if (i >= 20 && i <= 25) {
            cmd += " XXX.tga";
        }
        if (i == 13) { //filter-gauss-n
            cmd += " 3";
        }
        if (i == 19) { //size
            cmd += " 2";
        }
        if (i == 26) { //rotate
            cmd += " 30";
        }

        CScriptHandler::HandleCommand(cmd.c_str(), pImage);
        //CScriptHandler::HandleCommand(("save Result/" + cmds[i]).c_str(), pImage);
        CScriptHandler::HandleCommand(("load Sample_Results/" + cmds[i] + ".tga").c_str(), targetImage);
        if (!pImage) {
            std::cerr << cmds[i] << " : no pic" << std::endl;
            continue;
        }
        if (!targetImage) {
            std::cerr << cmds[i] << " : no sample" << std::endl;
            continue;
        }
        if (!pImage->Compare(targetImage)) {
            std::cerr << cmds[i] << " : wrong wrong" << std::endl;
        }
    }
}

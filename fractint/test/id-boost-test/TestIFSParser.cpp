#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>
#include "IFSParser.h"

static const std::string s_binary_definition =
	"\n"
	" binary { ; comment allowed here\n"
	"  ; and here\n"
	"  .5  .0 .0 .5 -2.563477 -0.000003 .333333   ; also comment allowed here\n"
	"  .5  .0 .0 .5	2.436544 -0.000003 .333333\n"
	"  .0 -.5 .5 .0	4.873085  7.563492 .333333\n"
	"  }\n";

static const std::string s_bad_definition =
	"\n"
	" goopy\n"
	"\n";

static const std::string s_3d_definition =
	"3dfern (3D) {\n"
	"   .00	.00 0 .0 .18 .0 0  0.0 0.00 0 0.0 0 .01\n"
	"   .85	.00 0 .0 .85 .1 0 -0.1 0.85 0 1.6 0 .85\n"
	"   .20 -.20 0 .2 .20 .0 0  0.0 0.30 0 0.8 0 .07\n"
	"  -.20	.20 0 .2 .20 .0 0  0.0 0.30 0 0.8 0 .07\n"
	"  }\n";

static const std::string s_ifs_file =
	"\n"
	" binary { ; comment allowed here\n"
	"  ; and here\n"
	"  .5  .0 .0 .5 -2.563477 -0.000003 .333333   ; also comment allowed here\n"
	"  .5  .0 .0 .5	2.436544 -0.000003 .333333\n"
	"  .0 -.5 .5 .0	4.873085  7.563492 .333333\n"
	"  }\n"
	"\n"
	"coral {\n"
	"  .307692 -.531469 -.461538 -.293706  5.401953 8.655175 .40\n"
	"  .307692 -.076923  .153846 -.447552 -1.295248 4.152990 .15\n"
	"  .000000  .545455  .692308 -.195804 -4.893637 7.269794 .45\n"
	"  }\n"
	"\n"
	"crystal {\n"
	"  .696970 -.481061 -.393939 -.662879 2.147003 10.310288 .747826\n"
	"  .090909 -.443182  .515152 -.094697 4.286558  2.925762 .252174\n"
	"  }\n"
	"\n"
	"dragon {\n"
	"  .824074 .281482 -.212346  .864198 -1.882290 -0.110607 .787473\n"
	"  .088272 .520988 -.463889 -.377778  0.785360  8.095795 .212527\n"
	"  }\n"
	"\n"
	"fern {\n"
	"    0	 0    0  .16 0	 0 .01\n"
	"   .85	.04 -.04 .85 0 1.6 .85\n"
	"   .2  -.26  .23 .22 0 1.6 .07\n"
	"  -.15	.28  .26 .24 0 .44 .07\n"
	"  }\n"
	"\n"
	"3dfern (3D) {\n"
	"   .00	.00 0 .0 .18 .0 0  0.0 0.00 0 0.0 0 .01\n"
	"   .85	.00 0 .0 .85 .1 0 -0.1 0.85 0 1.6 0 .85\n"
	"   .20 -.20 0 .2 .20 .0 0  0.0 0.30 0 0.8 0 .07\n"
	"  -.20	.20 0 .2 .20 .0 0  0.0 0.30 0 0.8 0 .07\n"
	"  }\n"
	"\n"
	"floor {\n"
	"  .0 -.5  .5 .0 -1.732366 3.366182 .333333\n"
	"  .5  .0  .0 .5 -0.027891 5.014877 .333333\n"
	"  .0  .5 -.5 .0  1.620804 3.310401 .333333\n"
	"  }\n"
	"\n"
	"koch3 {\n"
	"  .307692 -.000000  .000000  .294118  4.119164 1.604278 .151515\n"
	"  .192308 -.205882  .653846  .088235 -0.688840 5.978916 .253788\n"
	"  .192308  .205882 -.653846  .088235  0.668580 5.962514 .253788\n"
	"  .307692 -.000000  .000000  .294118 -4.136530 1.604278 .151515\n"
	"  .384615 -.000000  .000000 -.294118 -0.007718 2.941176 .189394\n"
	"  }\n"
	"\n"
	"spiral {\n"
	"   .787879 -.424242 .242424 .859848  1.758647 1.408065 .895652\n"
	"  -.121212  .257576 .151515 .053030 -6.721654 1.377236 .052174\n"
	"   .181818 -.136364 .090909 .181818  6.086107 1.568035 .052174\n"
	"  }\n"
	"\n"
	"swirl5 {\n"
	"   .745455 -.459091  .406061  .887121 1.460279 0.691072 .912675\n"
	"  -.424242 -.065152 -.175758 -.218182 3.809567 6.741476 .087325\n"
	"  }\n"
	"\n"
	"tree {\n"
	"    0	 0   0	.5   0	 0  .05\n"
	"  .42 -.42  .42 .42  0	.2  .4\n"
	"  .42  .42 -.42 .42  0	.2  .4\n"
	"   .1	 0   0	.1   0	.2  .15\n"
	"  }\n"
	"\n"
	"triangle {\n"
	"  .5  0  0  .5	0  0  .33\n"
	"  .5  0  0  .5	0  1  .33\n"
	"  .5  0  0  .5	1  1  .34\n"
	"  }\n"
	"\n"
	"zigzag2 {\n"
	"  -.632407 -.614815 -.545370 .659259 3.840822 1.282321 .888128\n"
	"  -.036111  .444444  .210185 .037037 2.071081 8.330552 .111872\n"
	"  }\n"
	"\n"
	"\n"
	"3dTetrahedron (3D) { ; by Alex Matulich\n"
	"  0.50	0  0  0  0.50  0  0  0	0.50  0.00  0.00  1.00	0.25\n"
	"  0.50	0  0  0  0.50  0  0  0	0.50  0.00  0.87 -0.50	0.25\n"
	"  0.50	0  0  0  0.50  0  0  0	0.50 -0.87 -0.50 -0.50	0.25\n"
	"  0.50	0  0  0  0.50  0  0  0	0.50  0.87 -0.50 -0.50	0.25\n"
	"}\n"
	"3d5Tetrahedron (3D) { ; by Alex Matulich\n"
	"  0.44	0  0  0  0.44  0  0  0	0.44  0.00  0.00  1.00	0.20\n"
	"  0.44	0  0  0  0.44  0  0  0	0.44  0.00  0.87 -0.50	0.20\n"
	"  0.44	0  0  0  0.44  0  0  0	0.44 -0.87 -0.50 -0.50	0.20\n"
	"  0.44	0  0  0  0.44  0  0  0	0.44  0.87 -0.50 -0.50	0.20\n"
	"  0.44	0  0  0  0.44  0  0  0	0.44  0.00  0.00  0.00	0.20\n"
	"}\n"
	"3dHexahedron (3D) { ; by Alex Matulich\n"
	"  0.44	0  0  0  0.44  0  0  0	0.44  0.00  0.00  0.90	0.20\n"
	"  0.44	0  0  0  0.44  0  0  0	0.44  0.87 -0.50  0.00	0.20\n"
	"  0.44	0  0  0  0.44  0  0  0	0.44 -0.87 -0.50  0.00	0.20\n"
	"  0.44	0  0  0  0.44  0  0  0	0.44  0.00  1.00  0.00	0.20\n"
	"  0.44	0  0  0  0.44  0  0  0	0.44  0.00  0.00 -0.90	0.20\n"
	"}\n"
	"3dCube (3D) { ; by Alex Matulich\n"
	"  0.35	0  0  0  0.35  0  0  0	0.35  1.00  1.00  1.00	0.12\n"
	"  0.35	0  0  0  0.35  0  0  0	0.35  1.00  1.00 -1.00	0.13\n"
	"  0.35	0  0  0  0.35  0  0  0	0.35  1.00 -1.00  1.00	0.12\n"
	"  0.35	0  0  0  0.35  0  0  0	0.35  1.00 -1.00 -1.00	0.13\n"
	"  0.35	0  0  0  0.35  0  0  0	0.35 -1.00  1.00  1.00	0.12\n"
	"  0.35	0  0  0  0.35  0  0  0	0.35 -1.00  1.00 -1.00	0.13\n"
	"  0.35	0  0  0  0.35  0  0  0	0.35 -1.00 -1.00  1.00	0.12\n"
	"  0.35	0  0  0  0.35  0  0  0	0.35 -1.00 -1.00 -1.00	0.13\n"
	"}\n"
	"3dOctahedron (3D) { ; by Alex Matulich\n"
	"  0.40	0  0  0  0.40  0  0  0	0.40  0.00  0.00  1.00	0.17\n"
	"  0.40	0  0  0  0.40  0  0  0	0.40  1.00  0.00  0.00	0.16\n"
	"  0.40	0  0  0  0.40  0  0  0	0.40  0.00  1.00  0.00	0.17\n"
	"  0.40	0  0  0  0.40  0  0  0	0.40 -1.00  0.00  0.00	0.17\n"
	"  0.40	0  0  0  0.40  0  0  0	0.40  0.00 -1.00  0.00	0.16\n"
	"  0.40	0  0  0  0.40  0  0  0	0.40  0.00  0.00 -1.00	0.17\n"
	"}\n"
	"3dDuodecahedron (3D) { ; by Alex Matulich\n"
	"  0.28	0  0  0  0.28  0  0  0	0.28  0.00  0.00  0.96	0.09\n"
	"  0.28	0  0  0  0.28  0  0  0	0.28  0.00  0.85  0.43	0.08\n"
	"  0.28	0  0  0  0.28  0  0  0	0.28  0.81  0.26  0.43	0.08\n"
	"  0.28	0  0  0  0.28  0  0  0	0.28 -0.81  0.26  0.43	0.09\n"
	"  0.28	0  0  0  0.28  0  0  0	0.28  0.50 -0.69  0.43	0.08\n"
	"  0.28	0  0  0  0.28  0  0  0	0.28 -0.50 -0.69  0.43	0.08\n"
	"  0.28	0  0  0  0.28  0  0  0	0.28  0.50  0.69 -0.43	0.09\n"
	"  0.28	0  0  0  0.28  0  0  0	0.28 -0.50  0.69 -0.43	0.08\n"
	"  0.28	0  0  0  0.28  0  0  0	0.28  0.81 -0.26 -0.43	0.08\n"
	"  0.28	0  0  0  0.28  0  0  0	0.28 -0.81 -0.26 -0.43	0.09\n"
	"  0.28	0  0  0  0.28  0  0  0	0.28  0.00 -0.85 -0.43	0.08\n"
	"  0.28	0  0  0  0.28  0  0  0	0.28  0.00  0.00 -0.96	0.08\n"
	"}\n"
	"\n"
	"fractint { ; by Pieter Branderhorst\n"
	"  0.00 -0.11  0.22  0.00 -6.25 4.84 0.06\n"
	"  0.11	0.02  0.00  0.11 -6.30 5.99 0.03\n"
	"  0.06	0.02  0.00  0.10 -6.25 4.51 0.02\n"
	"  0.00 -0.11  0.22  0.00 -4.34 4.84 0.06\n"
	"  0.08	0.00  0.00  0.11 -4.50 5.99 0.02\n"
	"  0.00	0.11 -0.08  0.00 -4.30 6.15 0.02\n"
	" -0.09	0.00 -0.01 -0.13 -4.15 5.94 0.02\n"
	"  0.06	0.11 -0.13  0.00 -4.69 4.15 0.04\n"
	"  0.03 -0.11  0.23  0.11 -2.26 4.43 0.07\n"
	"  0.03	0.11 -0.25  0.00 -2.57 4.99 0.07\n"
	"  0.06	0.00  0.00  0.11 -2.40 4.46 0.02\n"
	"  0.00	0.11 -0.19  0.00 -1.62 4.99 0.06\n"
	"  0.09 -0.01  0.00  0.10 -0.58 2.96 0.03\n"
	" -0.09	0.00  0.00 -0.11 -0.65 7.10 0.03\n"
	"  0.12	0.00 -0.00  0.11  1.24 6.00 0.03\n"
	"  0.00	0.11 -0.22  0.00  0.68 4.80 0.06\n"
	" -0.12	0.00  0.00 -0.13  6.17 7.18 0.03\n"
	"  0.00 -0.11  0.22  0.00  6.78 4.84 0.06\n"
	"  0.00	0.08 -0.25  0.02  2.21 4.95 0.07\n"
	"  0.00 -0.11  0.22  0.00  4.10 4.84 0.06\n"
	"  0.00 -0.11  0.22  0.00  5.25 5.23 0.06\n"
	"  0.08	0.11 -0.25  0.00  3.57 4.99 0.08\n"
	"  }\n"
	"\n";

BOOST_AUTO_TEST_CASE(IFSParser_ParseBinary)
{
	IFSParser parser = IFSParser::StackInstance();

	BOOST_CHECK(parser.Parse(s_binary_definition));
	BOOST_CHECK_EQUAL(1, parser.Count());
	const IFSEntry *entry = parser.Entry(0);
	BOOST_CHECK_EQUAL("binary", entry->Id());
	const int rowCount = 3;
	BOOST_CHECK_EQUAL(rowCount, long(entry->Transforms().size()));

	const int coefficientCount = 7;
	double data[rowCount][coefficientCount] =
	{
		{ .5, .0, .0, .5, -2.563477, -0.000003, .333333 },
		{ .5, .0, .0, .5, 2.436544, -0.000003, .333333 },
		{ .0, -.5, .5, .0, 4.873085, 7.563492, .333333 }
	};
	for (int row = 0; row < rowCount; row++)
	{
		const IFSTransformation *transform = entry->Transforms()[row];
		const double *coefficients = transform->GetCoefficients();
		for (int coefficient = 0; coefficient < coefficientCount; coefficient++)
		{
			BOOST_CHECK_CLOSE(coefficients[coefficient], data[row][coefficient], 1e-6);
		}
	}
}

BOOST_AUTO_TEST_CASE(IFSParser_ParseBad)
{
	IFSParser parser = IFSParser::StackInstance();

	BOOST_CHECK(!parser.Parse(s_bad_definition));
	BOOST_CHECK_EQUAL(0, parser.Count());
}

BOOST_AUTO_TEST_CASE(IFSParser_ParseFile)
{
	IFSParser parser = IFSParser::StackInstance();

	BOOST_CHECK(parser.Parse(s_ifs_file));
	BOOST_CHECK_EQUAL(20, parser.Count());
	BOOST_CHECK_EQUAL("binary", parser.Entry(0)->Id());
	BOOST_CHECK_EQUAL("fractint", parser.Entry(parser.Count()-1)->Id());
}

BOOST_AUTO_TEST_CASE(IFSParser_Parse3D)
{
	IFSParser parser = IFSParser::StackInstance();

	BOOST_CHECK(parser.Parse(s_3d_definition));
	BOOST_CHECK_EQUAL(1, parser.Count());

	const IFSEntry *entry = parser.Entry(0);
	BOOST_CHECK_EQUAL("3dfern", entry->Id());
	const int rowCount = 4;
	BOOST_CHECK_EQUAL(rowCount, long(entry->Transforms().size()));

	const int coefficientCount = 13;
	double data[rowCount][coefficientCount] =
	{
		{ .00, .00, 0, .0, .18, .0, 0, 0.0, 0.00, 0, 0.0, 0, .01 },
		{ .85, .00, 0, .0, .85, .1, 0, -0.1, 0.85, 0, 1.6, 0, .85 },
		{ .20, -.20, 0, .2, .20, .0, 0, 0.0, 0.30, 0, 0.8, 0, .07 },
		{ -.20, .20, 0, .2, .20, .0, 0, 0.0, 0.30, 0, 0.8, 0, .07 }
	};
	for (int row = 0; row < rowCount; row++)
	{
		const IFSTransformation *transform = entry->Transforms()[row];
		const double *coefficients = transform->GetCoefficients();
		for (int coefficient = 0; coefficient < coefficientCount; coefficient++)
		{
			BOOST_CHECK_CLOSE(coefficients[coefficient], data[row][coefficient], 1e-6);
		}
	}
}

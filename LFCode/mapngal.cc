
/*
 Project   LSST/BAO/PhotoZ
 Test program for computing galaxy number counts starting from MultiType_Z_LF , 
 and set of SED's and Filters 
 F. Habibi & R.Ansari ,  LAL (CNRS/IN2P3)  & Univ. Paris Sud
 Novembre 2017                                          -------  */

/*
 The code computes the limit on the absolute B mag for ellipiticals, spirals and star bursts galaxies,
 for a given apparent mag limit on r-band, at different redshifts.
 
 Then, the number density of the elliptical galexis are computed throuh integrating the Scheshter function
 with parameters given by Dahlen et al. 2005
 
 
 R. Ansari & F. Habibi Juillet 2016, Tir 1395, LAL

*/


#include <unistd.h>

#include <iostream>
#include <fstream>
#include <string>
#include <math.h>
#include <cmath>

#include "pexceptions.h"
#include "array.h"
#include "histats.h"
#include "ctimer.h"
#include "randfmt.h"
#include "randr48.h"
#include "samba.h"

#include "galcnt.h"

/*
#include "multitypzlf.h"
#include "sedfilter.h"
#include "kcorr.h"
*/

#include "luc.h"
#include "slininterp.h"
#include "integ.h"

#include "fitsioserver.h"





int main(int narg, char* arg[]) {
	
	cout << endl;
	cout << " -------------------- mapngal.cc ( MultiType_Z_LF & GalaxyCountComputer ) ----------------------- " << endl;
	cout << endl;
	
	string inlfname = "lfparamDahlenAll.txt";
	string outppfname = "galcnt.ppf";
	string outfitsname = "";
	string sedpath = "../SEDs/";
	string filtpath = "../Filters/";
	int prtlev=0;
	
	bool fguseall = false;
	bool fgLFfilB = true;
	double magMin=-27. , magMax=-13;
	int nbmagpts=50;
	double zmin = 0.1, zmax=4.1, dz=0.05;
	double lambdamin=300., lambdamax=1000.;  // in nanometre
	double lambdaSEDmin=100.;  // on nanometre, SED wavelength interval
	double lambdaSEDmax=2500.;
	double maglim = 25.3;  // Magnitude limit in observation band (LSST-i-band for example)
	double magerr = 0.;  // Error on magnitude , 0. -> NoError (used also for scan mag
	/*  (science book) double x = pow(10,.4*(maglim-m5i)); double gammai = 0.039;
		 magErr = sqrt( (.04-gammai)*x + gammai*x*x );  */
	double scanmagmin=23., scanmagmax=27., scanmagstep=0.2;
	// Fraction of the galaxies for each LF distributed over the 6 SED 
	double fEll[6]={1.,0.,0.,0.,0.,0.};   // for elliptical 
	bool fg_fEll=false;      //  true, -fEll specified 
	double fSp[6]={0.,0.4,0.4,0.2,0.,0.};   // for Spirals 
	bool fg_fSp=false;       //  true, -fSp specified 
	double fSB[6]={0.,0.,0.,0.,0.5,0.5};   // for StarBurst 
	bool fg_fSB=false;       //  true, -fSB specified 

	if ((narg>1)&&(strcmp(arg[1],"-h")==0)) {
		cout << " Test computation of ngal using MultiType_Z_LF & GalaxyCountComputer classes  Usage: \n"
		 << " Usage: mapngal [-i input_schech_param_file] [-magMLF magMin,magMax,nbpts] [-useall] \n"
		 << "              [-sedp SEDFilePath] [-filtp FilterPath] [-LFB] [-LFiS] \n"
		 << "              [-lambda min,max] [-lambdaSED min,max] \n"
		 << "              [-fEll f1,f2,f3,f4,f5,f6] [-fSp f1,f2,f3,f4,f5,f6] [-fSB f1,f2,f3,f4,f5,f6] \n"
		 << "              [-o outppfname] [-fits outfitsname] [-prt lev] \n" 
		 << "              [-limag magLimit,magErr] [-z zmin,zmax,dz] [-scanmag magmin,magmax,dmag] \n" << endl;
	cout << "  -i input_schech_param_file: input text file name containing Schechter LF parameters \n" 
		 << " -magMLF magMin,magMax,nbmagpts: min/max/step step absolute magnitudes in LF function (def=-27,-13,50) \n"
		 << " -useall: use the Scheshter parameters for total galaxies from the input file. False by default.\n"
		 << " -sedp sedpath: path for the SEDs directory. default sedpath=" << sedpath << "\n"
		 << " -filtp filtpath: path for the filters directory. default filtpath=" << filtpath << "\n"
		 << " -LFB: if the Scheshter parameters are given in B-band. True by default \n" 
		 << " -LFiS: if the Scheshter parameters are given in iS-band. False by default \n"
		 << " -lambda lambdamin,lambdamax : wavelength range (in nm) for filter definitions. Default 300,1000 nm \n"
		 << " -lambdaSED lambdamin,lambdamax : wavelength range (in nm) for SED's definitions. Default 100,2500 nm \n"
		 << " -fEll -fSp -fSB f1,f2,f3,f4,f5,f6 : Specify for each LF (Elliptical, Spiral, StarBurst) the fractional \n"
		 << "            distribution over the 6 SED's : El_cww, Scd_cww, Sbc_cww, Im_cww, SB2_kin, SB3_kin \n"
		 << " -limag maglim,magerr: apparent magnitude limit and mag-error in i-band for count(redshift) NTuple. default = 25.3,0.  \n" 
		 << " -z zmin,zmax,dz: redshift range. Default zmin=0.1, zmax=4.1 and dz=0.05 \n"
		 << " -scanmag magmin,magmax,dmag: magnitude limit scan range Default: 23,27,0.2 (Note: uses same magErr as -limag) \n" 
		 << " -o outppfname: output file name in ppf format. Default  = galcnt.ppf \n" 
		 << " -fits outfitsname: output file name in FITS format. Default  NONE \n" 
		 << " -prt lev: define print level. default=0 \n" << endl;		

		return 1;
	}
	
	
	bool fgoptarg=true;
	while (fgoptarg&&(narg>1))    {
	  string fbo = arg[1];
	  if (fbo=="-i")  {
	if (narg<3) { cout << " mapngal.cc: missing/bad argument, -h for help " << endl;  return -1; }
	inlfname = arg[2];   arg+=2; narg-=2;
	  }
	  else if (fbo=="-sedp")  {
	if (narg<3) { cout << " mapngal.cc: missing/bad argument, -h for help " << endl;  return -1; }
	sedpath = arg[2];   arg+=2; narg-=2;
	  }
	  else if (fbo=="-filtp")  {
	if (narg<3) { cout << " mapngal.cc: missing/bad argument, -h for help " << endl;  return -1; }
	filtpath=arg[2];   arg+=2; narg-=2;
	  }
	  else if (fbo=="-magMLF")  {
	if (narg<3) { cout << " mapngal.cc: missing/bad argument, -h for help " << endl;  return -1; }
	sscanf(arg[2],"%lg,%lg,%d",&magMin,&magMax,&nbmagpts);   arg+=2; narg-=2;
	  }
	  else if (fbo=="-limag")  {
	if (narg<3) { cout << " mapngal.cc: missing/bad argument, -h for help " << endl;  return -1; }
	sscanf(arg[2],"%lg,%lg",&maglim,&magerr);     arg+=2; narg-=2;
	  }
	  else if (fbo=="-z")  {
	if (narg<3) { cout << " mapngal.cc: missing/bad argument, -h for help " << endl;  return -1; }
	sscanf(arg[2],"%lg,%lg,%lg",&zmin,&zmax,&dz);   arg+=2; narg-=2;
	  }
	  else if (fbo=="-scanmag")  {
	if (narg<3) { cout << " mapngal.cc: missing/bad argument, -h for help " << endl;  return -1; }
	sscanf(arg[2],"%lg,%lg,%lg",&scanmagmin,&scanmagmax,&scanmagstep);   arg+=2; narg-=2;
	  }
	  else if (fbo=="-lambda")  {
	if (narg<3) { cout << " mapngal.cc: missing/bad argument, -h for help " << endl;  return -1; }
	sscanf(arg[2],"%lg,%lg",&lambdamin,&lambdamax);   arg+=2; narg-=2;
	  }
	  else if (fbo=="-lambdaSED")  {
	if (narg<3) { cout << " mapngal.cc: missing/bad argument, -h for help " << endl;  return -1; }
	sscanf(arg[2],"%lg,%lg",&lambdaSEDmin,&lambdaSEDmax);   arg+=2; narg-=2;
	  }
	  else if (fbo=="-fEll")  {
	if (narg<3) { cout << " mapngal.cc: missing/bad argument, -h for help " << endl;  return -1; }
	sscanf(arg[2],"%lg,%lg,%lg,%lg,%lg,%lg",fEll,fEll+1,fEll+2,fEll+3,fEll+4,fEll+5);  fg_fEll=true;  arg+=2; narg-=2;
	  }
	  else if (fbo=="-fSp")  {
	if (narg<3) { cout << " mapngal.cc: missing/bad argument, -h for help " << endl;  return -1; }
	sscanf(arg[2],"%lg,%lg,%lg,%lg,%lg,%lg",fSp,fSp+1,fSp+2,fSp+3,fSp+4,fSp+5);  fg_fSp=true;  arg+=2; narg-=2;
	  }
	  else if (fbo=="-fSB")  {
	if (narg<3) { cout << " mapngal.cc: missing/bad argument, -h for help " << endl;  return -1; }
	sscanf(arg[2],"%lg,%lg,%lg,%lg,%lg,%lg",fSB,fSB+1,fSB+2,fSB+3,fSB+4,fSB+5);  fg_fSB=true;  arg+=2; narg-=2;
	  }
	  else if (fbo=="-o")  {
	if (narg<3) { cout << " mapngal.cc: missing/bad argument, -h for help " << endl;  return -1; }
	outppfname=arg[2];   arg+=2; narg-=2;
	  }
	  else if (fbo=="-fits")  {
	if (narg<3) { cout << " mapngal.cc: missing/bad argument, -h for help " << endl;  return -1; }
	outfitsname=arg[2];   arg+=2; narg-=2;
	  }
	  else if (fbo=="-prt")  {
	if (narg<3) { cout << " mapngal.cc: missing/bad argument, -h for help " << endl;  return -1; }
	prtlev=atoi(arg[2]);   arg+=2; narg-=2;
	  }
	  else if (fbo=="-useall")  {
	fguseall=true; arg++; narg--;
	  }
	  else if (fbo=="-LFB")  {
	fgLFfilB=true; arg++; narg--;
	  }
	  else if (fbo=="-LFiS")  {
	fgLFfilB=false; arg++; narg--;
	  }
	  else {
	cout << " mapngal.cc wrong option "<<fbo<<" use -h to see list of options"<<endl;
	return 9;
	  }
	}
	
	cout << " Inpute LF file: " << inlfname << endl;
	cout << " Output file: " << outppfname << endl;
	cout << " Reading SDEs from: " << sedpath << endl;
	cout << " Reading Filters from: " << filtpath << endl;
	cout << " MultiType_Z_LF magnitude range magMin=" << magMin << ", magMax=" << magMax << endl;
	cout << " z-min=" << zmin << ", z-max=" << zmax << " z-step=" << dz << endl;
	// setting the lambda's interval for the spectra
	cout << " Wavelength interval(nm): [" << lambdamin << "," << lambdamax << "]" << endl;
	cout << " SED Wavelength interval(nm): [" << lambdaSEDmin << "," << lambdaSEDmax << "]" << endl;
	cout << " MagnitudeLimit (Observation band) = " << maglim << " MagnitudeError= "<< magerr <<endl;
	cout << " MagnitudeLimit Scan Range (Observation band) = " << scanmagmin << "," << scanmagmax << "," << scanmagstep << endl;
	cout << " Luminosity Functions filter = " << (fgLFfilB?"B-Filter":"iS-Filter") << endl;
	cout << " PrintLevel = " << prtlev << endl;

	// make sure SOPHYA modules are initialized
	SophyaInit();
	FitsIOServerInit();
	
	SFPathManager sedpathmgr(sedpath);
	SFPathManager filtpathmgr(filtpath);

	int rc = 0;
	
	double m5i = 24.+2.5;
	try {
	Timer tm("mapngal.cc");

	cout << "[1] Creating SimpleUniverse su(Planck2015LambdaCDM) ..."<<endl;
	SimpleUniverse su;   // Standard Planck LambdaCDM cosmology 
	MultiType_Z_LF::set_def_H0(su.H0());
	
	cout << "[2] Creating MultiType_Z_LF multLF(inlfname, MagMin, MagMax, nbmagpts, useall) ..."<<endl;
	MultiType_Z_LF multLF(inlfname, magMin, magMax, nbmagpts, fguseall);
	cout << " ... Creating and AutoInit of Random Generator ..."<<endl;
	FMTRandGen rg;
	rg.AutoInit(1);
	multLF.setRandomGenerator(&rg);
	cout << multLF;
	
	
	cout << "* magnitude limit=" << maglim << ", magErr=delmag=" << magerr << endl;

	 
	cout << "[3] Reading filters ..."<<endl;
	
	// importing the filter shapes
	// LF definition filters
	string filterfile_iS = filtpathmgr.BuildFileName("iS_cfht.txt");  // i' (iS) filter of CFHT telescope (Ramos et al. 2011, LF)
	cout << "* Reading Filter from file " << filterfile_iS << endl;
	Filter filt_iS(filterfile_iS, lambdamin*1e-9, lambdamax*1e-9);
	
	string filterfile_B = filtpathmgr.BuildFileName("B_wfi_eso.txt");  // ../filters/g_SDSS.txt");
	cout << "* Reading Filter from file " << filterfile_B << endl;
	Filter filt_B(filterfile_B, lambdamin*1e-9, lambdamax*1e-9);
	
	// Observation filters
	string filterfile_r = filtpathmgr.BuildFileName("r_lsst_etc.txt");  // ../filters/g_SDSS.txt");
	cout << "* Reading Filter from file " << filterfile_r <<endl;
	Filter filt_r(filterfile_r, lambdamin*1e-9, lambdamax*1e-9);
	
	string filterfile_i = filtpathmgr.BuildFileName("i_lsst_etc.txt");  // ../filters/g_SDSS.txt");
	cout << "* Reading Filter from file " << filterfile_i <<endl;
	Filter filt_i(filterfile_i, lambdamin*1e-9, lambdamax*1e-9);
	
	Filter & filt_LF = filt_B;  // this is for Dahlen
	if (!fgLFfilB)  filt_LF = filt_iS; // this is for Ramos
	
	Filter & filt_Obs = filt_i;  // we choose the observation filter in which magLimit is set




	cout << "[4] Creating GalaxyCountComputer object ..."<<endl;	    
	GalaxyCountComputer galcntc(multLF, filt_LF, filt_Obs, su, sedpathmgr, lambdaSEDmin, lambdaSEDmax);
	galcntc.setPrintLevel(prtlev);
	if (fg_fEll) {
		double * fLF = fEll;
		cout << "[4...] Changing fractional distribution over the 6 SED's for LF-Ellipticals\n"
		 << "   "<<fLF[0]<<","<<fLF[1]<<","<<fLF[2]<<","<<fLF[3]<<","<<fLF[4]<<","<<fLF[5]<<endl;
		std::vector<double> vfLF(6);  for(size_t jj=0; jj<6; jj++)  vfLF[jj]=fLF[jj];
		galcntc.setEllipticalSEDFraction(vfLF);
	}
	if (fg_fSp) {
		double * fLF = fSp;
		cout << "[4...] Changing fractional distribution over the 6 SED's for LF-Spirals\n"
		 << "   "<<fLF[0]<<","<<fLF[1]<<","<<fLF[2]<<","<<fLF[3]<<","<<fLF[4]<<","<<fLF[5]<<endl;
		std::vector<double> vfLF(6);  for(size_t jj=0; jj<6; jj++)  vfLF[jj]=fLF[jj];
		galcntc.setSpiralSEDFraction(vfLF);
	}
	if (fg_fSp) {
		double * fLF = fSB;
		cout << "[4...] Changing fractional distribution over the 6 SED's for LF-StarBurst\n"
		 << "   "<<fLF[0]<<","<<fLF[1]<<","<<fLF[2]<<","<<fLF[3]<<","<<fLF[4]<<","<<fLF[5]<<endl;
		std::vector<double> vfLF(6);  for(size_t jj=0; jj<6; jj++)  vfLF[jj]=fLF[jj];
		galcntc.setStarBurstSEDFraction(vfLF);
	}


	//Creating interpolated n_gal(E(B-V))
	cout << "[5] Interpolating n_gal ...";
	double Ellcnt, Spcnt, SBcnt;
	string txtfilename = "ngal_points.txt";
	
	//
	//vector<double> ngal_vector;
	//vector<double> ebmv_vector;
	//double ebmv;
	//double ebmv_max = 7.35;
	//double ebmv_min = 0.;
	//int ebmv_steps = 1000;
	
	//ofstream ngal_points;
	//ngal_points.open("ngal_points_f99.txt");
	
	//for(int ii=0; ii<ebmv_steps; ii++){
		//ebmv = (ebmv_max-ebmv_min)/ebmv_steps*ii+ebmv_min;
		//galcntc.doCompute(zmin, zmax, dz, maglim, magerr, lambdamin, lambdamax, ebmv);
		//ngal_vector.push_back(galcntc.getIntegratedGalDensity_Arcmin2(Ellcnt, Spcnt, SBcnt));
		//ebmv_vector.push_back(ebmv);
		//ngal_points << ebmv_vector[ii] << " " << ngal_vector[ii] << endl;
		//cout << ebmv_vector[ii] << endl;
	//}
	//ngal_points.close();
	
	//cout << "[[[[[[[[[[[]]]]]]]]]]" << endl;
	//SLinInterp1D ngal_interpolated(ebmv_vector, ngal_vector);
	//

	SLinInterp1D ngal_interpolated;
	SLinInterp1D ngal_interpolated_mod;
	ngal_interpolated.ReadXYFromFile("ngal_points.txt");
	//ngal_interpolated_mod.ReadXYFromFile("ngal_points_zucca.txt");



	//Loading the fit file
	/* Load two maps and return the galcnt map of the first or the second, mind the correction factor*/
	//cout << "[6] Loading dust map..." << endl;

	//FitsInOutFile fis1("../Dustmaps/cl_ps1_086.fits[1][col I]", FitsInOutFile::Fits_RO);
	//FitsInOutFile fis2("../Dustmaps/ps1.fits[1][col ebv]", FitsInOutFile::Fits_RO);
	//double correction_factor_sfd = 1.0;
	//double correction_factor_sfd_mod = 0.86;
	//SphereHEALPix<r_4> ebmv_map1;
	//SphereHEALPix<r_4> ebmv_map2;
	//SphereHEALPix<r_4> diff_map;

	//FitsManager::Read(fis1, ebmv_map1);
	//FitsManager::Read(fis2, ebmv_map2);
	//double totcnt_reddened;
	//double totcnt_reddened_mod;

	//diff_map.setNSide(ebmv_map2.SizeIndex(), ebmv_map2.IfRING());
	
	//cout << ebmv_map1.TypeOfMap() << endl;
	//cout << ebmv_map2.TypeOfMap() << endl;
	//cout << diff_map.TypeOfMap() << endl;
	
	//cout << "[7] Computing galaxy number map" << endl;
	
	//for(int ii = 0; ii<ebmv_map1.NbPixels(); ii++){
		//if (isnan(ebmv_map1[ii])) {cout << "nan" << endl;}
		//if (ebmv_map1[ii]==1e-33 or ebmv_map2[ii]==1e-33 or ebmv_map1[ii]<0 or ebmv_map2[ii]<0 or isnan(ebmv_map1[ii]) or isnan(ebmv_map2[ii])){
			//diff_map[ii] = -1.6375e+30;
		//}
		//else{
			//totcnt_reddened 	= ngal_interpolated(ebmv_map1[ii]*correction_factor_sfd);
			//totcnt_reddened_mod = ngal_interpolated(ebmv_map2[ii]*correction_factor_sfd_mod);
			//if (totcnt_reddened != 0){
				////diff_map[ii] = totcnt_reddened_mod/totcnt_reddened;
				//diff_map[ii] = totcnt_reddened;
			//}
			//else{
				//diff_map[ii] = -1.6375e+30;
			//}
		//}
	//}
	//FitsInOutFile fos("Objs/diffgalcnt.fits", FitsInOutFile::Fits_Create);
	//FitsManager::Write(fos, diff_map);

	
	FitsInOutFile fis1("../Dustmaps/sfd.fits[1][col TEMPERATURE]", FitsInOutFile::Fits_RO);
	FitsInOutFile fis2("../Dustmaps/ps1.fits[1][col ebv]", FitsInOutFile::Fits_RO);
	SphereHEALPix<r_4> ebv1;
	SphereHEALPix<r_4> ebv2;
	FitsManager::Read(fis1, ebv1);
	FitsManager::Read(fis2, ebv2);
	
	//Same NSIDE
	int_4 nside1 = ebv1.SizeIndex();
	int_4 nside2 = ebv2.SizeIndex();
	if(nside1 > nside2){
		ebv1.Resize(nside2);
	}
	else if(nside1 < nside2){
		ebv2.Resize(nside1);
	}
	 cout << ebv1.TypeOfMap() << endl;
	//Same Ordering
	if(ebv1.TypeOfMap() != ebv2.TypeOfMap()){
		SphereHEALPix<r_4> conv_ebv1(ebv1.SizeIndex(), ebv2.IfRING());
		conv_ebv1 = ebv1;
		//double value, phi, theta;
		//for(int e=0; e < ebv1.NbPixels(); e++){
			//ebv1.PixThetaPhi(e, *theta, *phi);
			//conv_ebv1[conv_ebv1.PixIndexSph(theta, phi)] = ebv1[e];
		//}
	}
	~ ebv1();
	SphereHEALPix<r_4> ebv1(conv_ebv1);
	~ conv_ebv1();
	cout << ebv1.TypeOfMap() << endl;
	
	//Difference
	SphereHEALPix<r_4> diff_ebv(ebv1.SizeIndex(), ebv1.IfRING());
	vector<int> mask_indices;
	for(int ii=0; ii<ebv1.NbPixels(); ii++){
		if(ebv1[ii]<0 or ebv2[ii]<0 or ebv1[ii]==1e-33 or ebv2[ii]==1e-33){
			diff_ebv[ii] = 0; //-1.6375e30;
			mask_indices.push_back(ii);
		}
		else{
			diff_ebv[ii] = ebv1[ii]-ebv2[ii];
		}
	}
	vecor<TVector> cls;
	
	int nside = ebv1.SizeIndex();
	int lmax = 3*nside-1;
	SphericalTransformServer<r_8> sts;
	Alm<double> alm_start(lmax);
	sts.DecomposeToAlm(diff_ebv, alm_start, lmax, 0.);
	SphereHEALPix<r_4> err_ebv(nside, true);
	SphereHEALPix<r_4> mod_ebv(nside, true);
	sts.GenerateFromAlm(err_ebv, nside, alm_start);
	
	for(int ii=0; ii<mod_ebv.NbPixels(); ii++){
		mod_ebv[ii] = abs(ebv2[ii] - err_ebv/2);
	}
	for(int e=0; e<mask_indices.size(); e++){
		mod_ebv[mask_indices[e]] = 0; //mask
	}
	cls.push_back(sts.DecomposeToCl(mod_ebv, lmax, 0.));
	
	}  // End of try bloc
	
	catch (PThrowable & exc) {  // catching SOPHYA exceptions
		cerr << " mapngal.cc: Catched Exception (PThrowable)"
		<< (string)typeid(exc).name()
		<< "\n --> exc.Msg= " << exc.Msg() << endl;
		rc = 99;
	}
	catch (std::exception & e) {  // catching standard C++ exceptions
		cerr << " mapngal.cc: Catched std::exception "  << " - what()= "
		<< e.what() << endl;
		rc = 98;
	}
	catch (...) {  // catching other exceptions
		cerr << " mapngal.cc: some other exception (...) was caught ! "
		<< endl;
		rc = 97;
	}
	cout << " ------------------------- End of mapngal.cc (Rc="<<rc<<") ------------------------ " << endl;
	return rc;	
}

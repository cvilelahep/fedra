#include "EdbPVRQuality.h"
using namespace std;

ClassImp(EdbPVRQuality)

//______________________________________________________________________________

EdbPVRQuality::EdbPVRQuality()
{
    // Default Constructor
    cout << "EdbPVRQuality::EdbPVRQuality()   Default Constructor"<<endl;
    Set0();
    Init();
    Help();
}

//______________________________________________________________________________

EdbPVRQuality::EdbPVRQuality(EdbPVRec* ali)
{
    // Default Constructor
    cout << "EdbPVRQuality::EdbPVRQuality(EdbPVRec* ali)   Constructor (does automatically all in one...)"<<endl;
    Set0();
    Init();
    Help();

    if (NULL == ali || ali->Npatterns()<1) {
        cout << "WARNING   EdbPVRQuality(EdbPVRec* ali)   ali is no good EdbPVRec object! Check!" << endl;
        return;
    }

    // Set ali as eAli_orig
    eAli_orig=ali;
    eIsSource=kTRUE;

    CheckEdbPVRec();
    Execute_ConstantBTDensity();
		CreateEdbPVRec();
}

//______________________________________________________________________________

EdbPVRQuality::EdbPVRQuality(EdbPVRec* ali,  Float_t BTDensityTargetLevel)
{
    // Default Constructor
    cout << "EdbPVRQuality::EdbPVRQuality(EdbPVRec* ali)   Constructor (does automatically all in one...)"<<endl;
    Set0();
    Init();
    Help();

    if (NULL == ali || ali->Npatterns()<1) {
        cout << "WARNING   EdbPVRQuality(EdbPVRec* ali)   ali is no good EdbPVRec object! Check!" << endl;
        return;
    }

    // Set ali as eAli_orig
    eAli_orig=ali;
    eIsSource=kTRUE;
		
		// Set BTDensityTargetLevel
		SetBTDensityLevel(BTDensityTargetLevel);
		cout << " GetBTDensityLevel() " << GetBTDensityLevel() << endl;

    CheckEdbPVRec();
    Execute_ConstantBTDensity();
		CreateEdbPVRec();
}

//______________________________________________________________________________

EdbPVRQuality::~EdbPVRQuality()
{
    // Default Destructor
    cout << "EdbPVRQuality::~EdbPVRQuality()"<<endl;

    delete 		eHistChi2W;
    delete 		eHistYX;
		delete 		eProfileBTdens_vs_PID;
}

//______________________________________________________________________________

void EdbPVRQuality::Set0()
{
    // Reset Values
    eAli_orig=NULL;
    eAli_modified=NULL;
    eIsSource=kFALSE;
    for (int i=0; i<2; i++) eCutMethodIsDone[i]=kFALSE;

		// Default BT density level for which the standard cutroutine
		// will be put:
    eBTDensityLevel=20; // #BT/mm2

		// Reset Default Geometry: 0 OperaGeometry, 1: MC Geometry
    eHistGeometry=0;

    eHistChi2W = new TH2F("eHistChi2W","eHistChi2W",40,0,40,90,0,3);
    eHistYX = new TH2F("eHistYX","eHistYX",100,0,1,100,0,1);
		
		eHistYX->Reset();
		eHistChi2W->Reset();

    for (int i=0; i<57; i++) {
        ePatternBTDensity_orig[i]=0;
        ePatternBTDensity_modified[i]=0;
        eCutp1[i]=0.15;
        eCutp0[i]=1.0; // Maximum Cut Value for const, BT dens
        eAggreementChi2WDistCut[i]=3.0;  // Maximum Cut Value for const, BT dens
    }

    eProfileBTdens_vs_PID = new TProfile("eProfileBTdens_vs_PID","eProfileBTdens_vs_PID",57,0,57,0,60);
		cout << "eProfileBTdens_vs_PID->GetBinWidth(1)" << eProfileBTdens_vs_PID->GetBinWidth(1) << endl;
//  nbinsx, Double_t xlow, Double_t xup, Option_t* option = ""
 
    // Default values for cosmics, taken from a brick data:
    eAggreementChi2CutMeanChi2=1.0;
    eAggreementChi2CutRMSChi2=0.3;
    eAggreementChi2CutMeanW=23;
    eAggreementChi2CutRMSW=3;
    return;
}

//______________________________________________________________________________

void EdbPVRQuality::Init()
{
    /// TEMPORARY
    if (eHistGeometry==0) SetHistGeometry_OPERA();
		cout << "EdbPVRQuality::Init()   /// TEMPORARY  SetHistGeometry_MC " <<  endl;
		SetHistGeometry_MC();
    return;
}


//______________________________________________________________________________
void EdbPVRQuality::SetCutMethod(Int_t CutMethod)
{
    eCutMethod=CutMethod;
    cout << "EdbPVRQuality::SetCutMethod  eCutMethod=  " << eCutMethod << endl;
    if (CutMethod>1) {
        cout << "WARNING   EdbPVRQuality::SetCutMethod  eCutMethod invalid, Set back to default eCutMethod=  " << eCutMethod << endl;
    }
    return;
}



//______________________________________________________________________________

void EdbPVRQuality::CheckEdbPVRec()
{
    if (!eIsSource) {
			cout << "EdbPVRQuality::CheckEdbPVRec  eIsSource=  " << eIsSource << ". This means no source set. Return!" << endl;
			return;
		}
    // Check the patterns of the EdbPVRec:
    int Npat = eAli_orig->Npatterns();
    TH1F* histPatternBTDensity = new TH1F("histPatternBTDensity","histPatternBTDensity",100,0,100);

    for (int i=0; i<Npat; i++) {
        eHistYX->Reset(); // important to clean the histogram
        eHistChi2W->Reset(); // important to clean the histogram

        EdbPattern* pat = (EdbPattern*)eAli_orig->GetPattern(i);
        Int_t npat=pat->N();
        EdbSegP* seg=0;
        for (int j=0; j<npat; j++) {
					seg=(EdbSegP*)pat->At(j);
					// Very important:
					// For the data case, we assume the following:
					// Data (MCEvt==-999) will be taken for BTdens calculation
					// Sim (MCEvt>0) will not be taken for BTdens calculation
					if (seg->MCEvt() > 0) continue;
					
          eHistYX->Fill(seg->Y(),seg->X());
          eHistChi2W->Fill(seg->W(),seg->Chi2());
        }

        histPatternBTDensity->Reset();
        int nbins=eHistYX->GetNbinsX()*eHistYX->GetNbinsY();

				int bincontentXY=0;
        for (int k=1; k<nbins-1; k++) {
            if (eHistYX->GetBinContent(k)==0) continue;
            bincontentXY=eHistYX->GetBinContent(k);
            histPatternBTDensity->Fill(bincontentXY);
						eProfileBTdens_vs_PID->Fill(i,bincontentXY);
        }

        ePatternBTDensity_orig[i]=histPatternBTDensity->GetMean();
        histPatternBTDensity->Reset();
    }
    
    eHistYX->Reset();
		eHistChi2W->Reset();

    Print();
    return;
}



//______________________________________________________________________________
void EdbPVRQuality::SetHistGeometry_OPERA()
{
    eHistYX->Reset();
    eHistYX->SetBins(100,0,100000,120,0,120000);
    cout << eHistYX->GetBinWidth(1) << endl;
    return;
}
//______________________________________________________________________________
void EdbPVRQuality::SetHistGeometry_MC()
{
    eHistYX->Reset();
    eHistYX->SetBins(100,-50000,50000,100,-50000,50000);
    cout << eHistYX->GetBinWidth(1) << endl;
    return;
}

//______________________________________________________________________________
void EdbPVRQuality::Print() {

    cout << "----------void EdbPVRQuality::Print()----------" << endl;
    cout << " eCutMethodIsDone[0] " << eCutMethodIsDone[0] << endl;
    cout << " eCutMethodIsDone[1] " << eCutMethodIsDone[1] << endl;

    if (eCutMethodIsDone[0]) PrintCutType0();
    if (eCutMethodIsDone[1]) PrintCutType1();

    return;
}


//______________________________________________________________________________
void EdbPVRQuality::PrintCutType0()
{
    if (!eIsSource) return;
    cout << "----------void EdbPVRQuality::PrintCutType0()----------" << endl;
    cout << "Pattern || Z() || Nseg || eCutp0[i] || eCutp1[i] || BTDensity_orig ||...|| Nseg_modified || BTDensity_modified ||"<< endl;

    int Npat_orig = eAli_orig->Npatterns();

    if (NULL==eAli_modified) {
        eAli_modified=eAli_orig;
        cout << "WARNING eAli_modified==NULL  ==>>  Take eAli_orig instead. To calculate eAli_modified please run (not supported yet...)" << endl;
    }

    for (int i=0; i<Npat_orig; i++) {

        EdbPattern* pat_orig = (EdbPattern*)eAli_orig->GetPattern(i);
        Int_t npatO=pat_orig->N();
        EdbPattern* pat_modified = (EdbPattern*)eAli_modified->GetPattern(i);
        Int_t npatM=pat_modified->N();

        cout << i;
        cout << "	";
        printf("%.1f  %d  %.3f  %.3f  %.1f",pat_orig->Z(),npatO, eCutp0[i], eCutp1[i] , ePatternBTDensity_orig[i]);
        cout << "	...	";
        printf("%.1f  %d  %.3f  %.3f  %.1f",pat_modified->Z(),npatM,  eCutp0[i] ,eCutp1[i],  ePatternBTDensity_modified[i]);
        cout << endl;

    }

    return;
}

//______________________________________________________________________________
void EdbPVRQuality::PrintCutType1()
{
    if (!eIsSource) return;
    cout << "----------void EdbPVRQuality::PrintCutType1()----------" << endl;

    cout << "Pattern || Z() || Nseg || BTDensity_orig || Chi2CutMeanChi2 || Chi2CutRMSChi2 || Chi2CutMeanW || Chi2CutRMSW || Chi2Cut[i] || BTDensity_modified ||"<< endl;

    int Npat_orig 		= eAli_orig->Npatterns();

    if (NULL==eAli_modified) {
        eAli_modified=eAli_orig;
        cout << "WARNING eAli_modified==NULL  ==>>  Take eAli_orig instead. To calculate eAli_modified please run (not supported yet...)" << endl;
    }

    for (int i=0; i<Npat_orig; i++) {

        EdbPattern* pat_orig = (EdbPattern*)eAli_orig->GetPattern(i);
        Int_t npatO=pat_orig->N();
        EdbPattern* pat_modified = (EdbPattern*)eAli_modified->GetPattern(i);
        Int_t npatM=pat_modified->N();

        cout << i;
        cout << "	";
        printf("%.1f  %d  %.2f  %.2f  %.2f  %.2f  %.2f  %.1f",pat_orig->Z(),npatO, eAggreementChi2CutMeanChi2 , eAggreementChi2CutRMSChi2,  eAggreementChi2CutMeanW , eAggreementChi2CutRMSW, eAggreementChi2WDistCut[i], ePatternBTDensity_orig[i]);
        cout << "	...	";
        printf("%.1f  %d  %.2f  %.2f  %.2f  %.2f  %.2f  %.1f",pat_modified->Z(),npatM, eAggreementChi2CutMeanChi2 , eAggreementChi2CutRMSChi2,  eAggreementChi2CutMeanW , eAggreementChi2CutRMSW, eAggreementChi2WDistCut[i], ePatternBTDensity_modified[i]);
        cout << endl;

    }

    return;
}

//______________________________________________________________________________

void EdbPVRQuality::Execute_ConstantBTDensity()
{
    if (!eIsSource) return;
    cout << "----------void EdbPVRQuality::Execute_ConstantBTDensity()----------" << endl;

    int Npat = eAli_orig->Npatterns();
		cout << "----------void EdbPVRQuality::Execute_ConstantBTDensity()   Npat=" << Npat << endl;
		
    TH1F* histPatternBTDensity = new TH1F("histPatternBTDensity","histPatternBTDensity",100,0,100);

    for (int i=0; i<Npat; i++) {
        if (i>56) {
            cout << "ERROR     EdbPVRQuality::Execute_ConstantBTDensity() Your EdbPVRec object has more than 57 plates! " << endl;
            cout << "ERROR     EdbPVRQuality::Execute_ConstantBTDensity() Check it! " << endl;
            break;
        }
        cout << "Execute_ConstantBTDensity   Doing Pattern " << i << endl;

        // Now the condition loop:
        // Loop over 20 steps a 0.15,0.145,0.14 ...  down to
        for (int l=0; l<20; l++) {

            eHistYX->Reset(); // important to clean the histogram
            eHistChi2W->Reset(); // important to clean the histogram
            histPatternBTDensity->Reset(); // important to clean the histogram

            EdbPattern* pat = (EdbPattern*)eAli_orig->GetPattern(i);
            Int_t npat=pat->N();
            EdbSegP* seg=0;
            for (int j=0; j<npat; j++) {
                seg=(EdbSegP*)pat->At(j);

								// Very important:
								// For the data case, we assume the following:
								// Data (MCEvt==-999) will be taken for BTdens calculation
								// Sim (MCEvt>0) will not be taken for BTdens calculation
								if (seg->MCEvt() > 0) continue;

                // Constant BT density cut:
                if (seg->Chi2() >= seg->W()* eCutp1[i] - eCutp0[i]) continue;

                eHistYX->Fill(seg->Y(),seg->X());
                eHistChi2W->Fill(seg->W(),seg->Chi2());
            }


            int nbins=eHistYX->GetNbinsX()*eHistYX->GetNbinsY();

            for (int k=1; k<nbins-1; k++) {
                if (eHistYX->GetBinContent(k)==0) continue;
                histPatternBTDensity->Fill(eHistYX->GetBinContent(k));
            }

            ePatternBTDensity_modified[i]=histPatternBTDensity->GetMean();
            cout <<"Execute_ConstantBTDensity      Loop l= " << l << ":  for the eCutp1[i] : " << eCutp1[i] <<   "  we have a dens: "  << ePatternBTDensity_modified[i] << endl;

            // Now the condition check:
            if (ePatternBTDensity_modified[i]<=eBTDensityLevel) {
                cout << "Execute_ConstantBTDensity      We reached the loop end due to good BT density level ... and break loop." << endl;
                break;
            }
            else {
                // Tighten cut:
                eCutp1[i] += -0.005;
            }

        } // of condition loop...

    } // of Npattern loops..

    eCutMethodIsDone[0]=kTRUE;

		
		// This will be commented when using in batch mode...
		// For now its there for clarity reasons.
    TCanvas* c1 = new TCanvas();
    c1->Divide(2,2);
    c1->cd(1);
    eHistYX->DrawCopy("colz");
    c1->cd(2);
    eHistChi2W->DrawCopy("colz");
    c1->cd(3);
    histPatternBTDensity->DrawCopy("");
		c1->cd(4);
		eProfileBTdens_vs_PID->Draw("profileZ");
		c1->cd();
    histPatternBTDensity->Reset();
    eHistYX->Reset();
    eHistChi2W->Reset();

    delete histPatternBTDensity;
    return;
}

//______________________________________________________________________________


void EdbPVRQuality::Execute_ConstantQuality()
{
    if (!eIsSource) return;
    cout << "----------void EdbPVRQuality::Execute_ConstantQuality()----------" << endl;

    cout << "----------    Worsk for tracks passing the volume to extract their mean chi2/W " << endl;
    cout << "----------    If eAli->Tracks is there we take them from there." << endl;
    cout << "----------    If not w try if there is  a file linkedtracks.root " << endl;

    cout << "eAli_orig.eTracks :" << eAli_orig->eTracks << endl;

    Float_t meanChi2=0.5;
    Float_t rmsChi2=0.2;
    Float_t meanW=22;
    Float_t rmsW=4;
    Float_t aggreementChi2=100;

    // No  eAli.Tracks ? Look for tracks in linked_track.root:
    if (NULL == eAli_orig->eTracks) {
        TFile* trackfile = new TFile("linked_tracks.root");
        trackfile->ls();
        TTree* tracks = (TTree*)trackfile->Get("tracks");
// 		TH1F* h1;
        tracks->Draw("nseg>>h(60,0,60)","","");
        TH1F *h1 = (TH1F*)gPad->GetPrimitive("h");

        // Short Implementation of getting the last bin filled:
        int lastfilledbin=0;
        for (int k=1; k<h1->GetNbinsX()-1; k++) {
            if (h1->GetBinContent(k)>0) lastfilledbin=k;
        }

        TString cutstring = TString(Form("nseg>=%d",int(h1->GetBinCenter(lastfilledbin-3)) ));
        tracks->Draw("s.eChi2>>hChi2(100,0,2)",cutstring);
        TH1F *hChi2 = (TH1F*)gPad->GetPrimitive("hChi2");
        cout << hChi2->GetMean() << " " << hChi2->GetRMS() << endl;

        TCanvas* c1 = new TCanvas();
        tracks->Draw("s.eW>>hW(50,0,50)",cutstring);
        TH1F *hW = (TH1F*)gPad->GetPrimitive("hW");
        cout << hW->GetMean() << " " << hW->GetRMS() << endl;

        meanChi2=hChi2->GetMean();
        rmsChi2=hChi2->GetRMS();
        meanW=hW->GetMean();
        rmsW=hW->GetRMS();

        // since this is calcualted for the whole Volume,
        // this is valid for all plates.
        eAggreementChi2CutMeanChi2=meanChi2;
        eAggreementChi2CutRMSChi2=rmsChi2;
        eAggreementChi2CutMeanW=meanW;
        eAggreementChi2CutRMSW=rmsW;
    }

    /// ______  now same code as in the funciton above __________________________________

    int Npat = eAli_orig->Npatterns();
    TH1F* histPatternBTDensity = new TH1F("histPatternBTDensity","histPatternBTDensity",100,0,100);
    TH1F* histaggreementChi2 = new TH1F("histaggreementChi2","histaggreementChi2",100,0,5);

    for (int i=0; i<Npat; i++) {
        if (i>56) {
            cout << "ERROR     EdbPVRQuality::Execute_ConstantBTDensity() Your EdbPVRec object has more than 57 plates! " << endl;
            cout << "ERROR     EdbPVRQuality::Execute_ConstantBTDensity() Check it! " << endl;
            break;
        }

        cout << "Execute_ConstantQuality   Doing Pattern " << i << endl;

        // Now the condition loop:
        // Loop over 30 steps aggreementChi2 step 0.05
        for (int l=0; l<30; l++) {

            eHistYX->Reset(); // important to clean the histogram
            eHistChi2W->Reset(); // important to clean the histogram
            histPatternBTDensity->Reset(); // important to clean the histogram

            EdbPattern* pat = (EdbPattern*)eAli_orig->GetPattern(i);
            Int_t npat=pat->N();
            EdbSegP* seg=0;
            for (int j=0; j<npat; j++) {
                seg=(EdbSegP*)pat->At(j);

								// Very important:
								// For the data case, we assume the following:
								// Data (MCEvt==-999) will be taken for BTdens calculation
								// Sim (MCEvt>0) will not be taken for BTdens calculation
								if (seg->MCEvt() > 0) continue;

                // Constant BT density cut:
                //if (seg->Chi2() >= seg->W()* eCutp1[i] - eCutp0[i]) continue;
                // Constant BT quality cut:
                aggreementChi2=TMath::Sqrt( ( (seg->Chi2()-meanChi2)/rmsChi2)*((seg->Chi2()-meanChi2)/rmsChi2)  +   ((seg->W()-meanW)/rmsW)*((seg->W()-meanW)/rmsW) );

                histaggreementChi2->Fill(aggreementChi2);

                if (aggreementChi2>eAggreementChi2WDistCut[i]) continue;

                eHistYX->Fill(seg->Y(),seg->X());
                eHistChi2W->Fill(seg->W(),seg->Chi2());
            }

            int nbins=eHistYX->GetNbinsX()*eHistYX->GetNbinsY();

            for (int k=1; k<nbins-1; k++) {
                if (eHistYX->GetBinContent(k)==0) continue;
                histPatternBTDensity->Fill(eHistYX->GetBinContent(k));
                //cout <<"(eHistYX->GetBinContent(k)" <<eHistYX->GetBinContent(k) << endl;
            }

            ePatternBTDensity_modified[i]=histPatternBTDensity->GetMean();
            cout <<"Execute_ConstantBTDensity      Loop l= " << l << ":  for the eAggreementChi2WDistCut : " << eAggreementChi2WDistCut[i] <<   "  we have a dens: "  << ePatternBTDensity_modified[i] << endl;

            // Now the condition check:
            if (ePatternBTDensity_modified[i]<=eBTDensityLevel) {
                cout << "Execute_ConstantBTDensity      We reached the loop end due to good BT density level ... and break loop." << endl;
                // But dont forget to set values:
                eCutDistChi2[i]=meanChi2;
                eCutDistW[i]=meanW;
                eAggreementChi2CutMeanChi2=meanChi2;
                eAggreementChi2CutRMSChi2=rmsChi2;
                eAggreementChi2CutMeanW=meanW;
                eAggreementChi2CutRMSW=rmsW;
                break;
            }
            else {
                // Tighten cut:
                eAggreementChi2WDistCut[i]+=  -0.05;
            }

        } // of condition loop...

    } // of Npattern loops..

    eCutMethodIsDone[1]=kTRUE;

    histPatternBTDensity->Reset();
    eHistYX->Reset();
    eHistChi2W->Reset();
    delete histPatternBTDensity;
    return;
}


//___________________________________________________________________________________

Bool_t EdbPVRQuality::CheckSegmentQualityInPattern_ConstBTDens(EdbPVRec* ali, Int_t PatternAtNr, EdbSegP* seg)
{
    // Note: since the eCutp1[i] values are calculated with
    // this pattern->At() scheme labelling,
    // its not necessaryly guaranteed that seg->PID gives correct this
    // number back. Thats why we have to give the PatternAtNr here again.
    //
    // And: it is not checked here if seg is contained in this specific
    // pattern. It looks only for the quality cut!
    if (gEDBDEBUGLEVEL>3)  cout << "seg->W()* eCutp1[PatternAtNr] - eCutp0[PatternAtNr] = " << seg->W()* eCutp1[PatternAtNr] - eCutp0[PatternAtNr] << endl;

// Constant BT density cut:
    if (seg->Chi2() >= seg->W()* eCutp1[PatternAtNr] - eCutp0[PatternAtNr]) return kFALSE;

    if (gEDBDEBUGLEVEL>3) cout <<"EdbPVRQuality::CheckSegmentQualityInPattern_ConstBTDens()   Segment " << seg << " has passed ConstBTDens cut!" << endl;
    return kTRUE;
}

//___________________________________________________________________________________

Bool_t EdbPVRQuality::CheckSegmentQualityInPattern_ConstQual(EdbPVRec* ali, Int_t PatternAtNr, EdbSegP* seg)
{
    // See comments in CheckSegmentQualityInPattern_ConstBTDens
    // Constant BT quality cut:
    Float_t aggreementChi2=TMath::Sqrt( ( (seg->Chi2()-eAggreementChi2CutMeanChi2)/eAggreementChi2CutRMSChi2)*((seg->Chi2()-eAggreementChi2CutMeanChi2)/eAggreementChi2CutRMSChi2)  +   ((seg->W()-eAggreementChi2CutMeanW)/eAggreementChi2CutRMSW)*((seg->W()-eAggreementChi2CutMeanW)/eAggreementChi2CutRMSW) );
    if (aggreementChi2>eAggreementChi2WDistCut[PatternAtNr]) return kFALSE;

    if (gEDBDEBUGLEVEL>3) cout <<"EdbPVRQuality::CheckSegmentQualityInPattern_ConstQual()   Segment " << seg << " has passed ConstQual cut!" << endl;
    return kTRUE;
}

//___________________________________________________________________________________


void EdbPVRQuality::CreateEdbPVRec() {

    cout << "-----     ----------------------------------------------" << endl;
    cout << "-----     void EdbPVRQuality::CreateEdbPVRec()     -----" << endl;
    cout << "-----     This function makes out of the original eAli" << endl;
    cout << "-----     a new EdbPVRec object having only those seg-" << endl;
    cout << "-----     ments in it which satisfy the cutcriteria " << endl;
    cout << "-----     determined in Execute_ConstantBTDensity, Execute_ConstantQuality" << endl;
    cout << "-----     " << endl;
    cout << "-----     WARNING: the couples structure and the tracking structure" << endl;
    cout << "-----     will be lost, this PVR object is only useful for the" << endl;
    cout << "-----     list of Segments (==ShowReco...) ... " << endl;
    cout << "-----     DO NOT USE THIS ROUTINE FOR GENERAL I/O and/or EdbPVRec operations!" << endl;
    cout << "-----     " << endl;
    cout << "CreateEdbPVRec()  Mode 0:" << eCutMethodIsDone[0] << endl;
    cout << "CreateEdbPVRec()  Mode 1:" << eCutMethodIsDone[1] << endl;
    cout << "-----     " << endl;
		cout << "-----     ----------------------------------------------" << endl;

    if (NULL==eAli_orig || eIsSource==kFALSE) {
        cout << "WARNING!   NULL==eAli_orig   || eIsSource==kFALSE   return."<<endl;
        return;
    }

    if (eCutMethodIsDone[0]==kFALSE && eCutMethodIsDone[1]==kFALSE) {
        cout << "WARNING!   eCutMethodIsDone[0]==kFALSE && eCutMethodIsDone[1]==kFALSE   return."<<endl;
        return;
    }

    // Make a new PVRec object anyway
    eAli_modified = new EdbPVRec();

    // These two lines dont compile yet ... (???) ...
    // 	EdbScanCond* scancond = eAli_orig->GetScanCond();
    // 	eAli_modified->SetScanCond(*scancond);

    Float_t aggreementChi2;

    // This makes pointer copies of patterns with segments list.
    // wARNING: the couples structure and the tracking structure
    // will be lost, this PVR object is only useful for the
    // list of Segments (==ShowReco...) ...

    // Priority has the Execute_ConstantQuality cut.
    // If this was done we take this...
    for (int i = 0; i <eAli_orig->Npatterns(); i++ ) {
        EdbPattern* pat = eAli_orig->GetPattern(i);
        EdbPattern* pt= new EdbPattern();
        // SetPattern Values to the parent patterns:
        pt->SetID(pat->ID());
        pt->SetPID(pat->PID());
        pt->SetZ(pat->Z());

        for (int j = 0; j <pat->N(); j++ ) {
            EdbSegP* seg = pat->GetSegment(j);

            // Put here the cut condition ...
            if (eCutMethodIsDone[0]==kTRUE && eCutMethodIsDone[1]==kFALSE) {
                // Constant BT density cut:
                if (seg->Chi2() >= seg->W()* eCutp1[i] - eCutp0[i]) continue;
            }
            else if (eCutMethodIsDone[1]==kTRUE) {
                // Constant Quality cut:
                aggreementChi2=TMath::Sqrt( ( (seg->Chi2()-eAggreementChi2CutMeanChi2)/eAggreementChi2CutRMSChi2)*((seg->Chi2()-eAggreementChi2CutMeanChi2)/eAggreementChi2CutRMSChi2)  +   ((seg->W()-eAggreementChi2CutMeanW)/eAggreementChi2CutRMSW)*((seg->W()-eAggreementChi2CutMeanW)/eAggreementChi2CutRMSW) );
                if (aggreementChi2>eAggreementChi2WDistCut[i]) continue;
            }
            else {
                // do nothing;
            }

            // Add segment:
            pt->AddSegment(*seg);
        }
        eAli_modified->AddPattern(pt);
    }

    eAli_orig->Print();
    eAli_modified->Print();

    cout << "-----     void EdbPVRQuality::CreateEdbPVRec()...done." << endl;
    return;
}


//___________________________________________________________________________________

void EdbPVRQuality::Help()
{
    cout << "----------------------------------------------" << endl;
    cout << "-----     void EdbPVRQuality::Help()     -----" << endl;
    cout << "-----" << endl;
    cout << "-----     This Class helps you to determine the Quality Cut Plate by Plate" << endl;
    cout << "-----     for suited BG level for shower reco." << endl;
    cout << "-----     You use it like this:" << endl;
    cout << "-----        EdbPVRQuality* QualityClass = new EdbPVRQuality(gAli)" << endl;
    cout << "-----     where  gAli is an EdbPVRec object pointer (usually the one" << endl;
    cout << "-----     from the cp files). Then you can get the CutValues with" << endl;
    cout << "-----     GetCutp0() (for each//all plate) back." << endl;
    cout << "-----" << endl;
    cout << "-----     After holidays it will be interfaced with the libShower reconstruction mode." << endl;
    cout << "-----" << endl;
    cout << "-----     void EdbPVRQuality::Help()     -----" << endl;
    cout << "----------------------------------------------" << endl;
}

//___________________________________________________________________________________



EdbPVRec* EdbPVRQuality::Remove_DoubleBT(EdbPVRec* aliSource) {

    /// Quick and Dirty implementations !!!

    cout << "-----     void EdbPVRQuality::Remove_DoubleBT()" << endl;
    cout << "-----     void EdbPVRQuality::Take source EdbPVRec from " << aliSource << endl;

    EdbPVRec* eAli_source=aliSource;

    if (NULL==aliSource) {
        cout << "-----     void EdbPVRQuality::Source EdbPVRec is NULL. Change to object eAli_orig: " << eAli_orig << endl;
        eAli_source=eAli_orig;
    }

    if (NULL==eAli_orig) {
        cout << "-----     void EdbPVRQuality::Also eAli_orig EdbPVRec is NULL. Do nothing and return NULL pointer!" << endl;
        return NULL;
    }

    // Make a new PVRec object anyway
    EdbPVRec* eAli_target = new EdbPVRec();
    eAli_target->Print();

    Bool_t seg_seg_close=kFALSE;
    EdbSegP* seg=0;
    EdbSegP* seg1=0;
		Int_t NdoubleFoundSeg=0;

    for (int i = 0; i <eAli_source->Npatterns(); i++ ) {
        if (gEDBDEBUGLEVEL>2) cout << "Looping over eAli_source->Pat()=" << i << endl;

        EdbPattern* pat = eAli_source->GetPattern(i);
        EdbPattern* pt= new EdbPattern();
        // SetPattern Values to the parent patterns:
        pt->SetID(pat->ID());
        pt->SetPID(pat->PID());
        pt->SetZ(pat->Z());

        for (int j = 0; j <pat->N()-1; j++ ) {
            seg = pat->GetSegment(j);
            seg_seg_close=kFALSE;
            for (int k = j+1; k <pat->N(); k++ ) {
								if (seg_seg_close) continue;

                if (gEDBDEBUGLEVEL>3) cout << "Looping over eTracks for segment pair nr=" << j << "," << k << endl;
                seg1 = pat->GetSegment(k);

                // Here decide f.e. which segments to check...
                if (TMath::Abs(seg->X()-seg1->X())>2.1) continue;
                if (TMath::Abs(seg->Y()-seg1->Y())>2.1) continue;
                if (TMath::Abs(seg->TX()-seg1->TX())>0.01) continue;
                if (TMath::Abs(seg->TY()-seg1->TY())>0.01) continue;
                if (gEDBDEBUGLEVEL>3) cout << "EdbPVRQuality::Remove_DoubleBT()   Found compatible segment!! " << endl;
								++NdoubleFoundSeg;
                seg_seg_close=kTRUE;
//                 if (seg_seg_close) break;
            }
            if (seg_seg_close) continue;

            // Add segment:
            if (gEDBDEBUGLEVEL>3) cout << "// Add segment:" << endl;
            pt->AddSegment(*seg);
        }
        if (gEDBDEBUGLEVEL>2) cout << "// Add AddPattern:" << endl;
        eAli_target->AddPattern(pt);
    }

    if (gEDBDEBUGLEVEL>1) eAli_source->Print();
    if (gEDBDEBUGLEVEL>1) eAli_target->Print();

		cout << "-----     void EdbPVRQuality::Remove_DoubleBT()...Statistics: We found " << NdoubleFoundSeg  << " double segments too close to each other to be different BTracks." << endl;
    cout << "-----     void EdbPVRQuality::Remove_DoubleBT()...done." << endl;
    return eAli_target;
}
//___________________________________________________________________________________



EdbPVRec* EdbPVRQuality::Remove_Passing(EdbPVRec* aliSource)
{

    /// Quick and Dirty implementations !!!

    EdbPVRec* eAli_source=aliSource;

    if (NULL==aliSource) {
        cout << "-----     void EdbPVRQuality::Source EdbPVRec is NULL. Change to object eAli_orig: " << eAli_orig << endl;
        eAli_source=eAli_orig;
    }

    if (NULL==eAli_orig) {
        cout << "-----     void EdbPVRQuality::Also eAli_orig EdbPVRec is NULL. Do nothing and return NULL pointer!" << endl;
        return NULL;
    }

    TObjArray* Tracks=eAli_source->eTracks;
    Int_t TracksN=eAli_source->eTracks->GetEntries();
    EdbTrackP* track;
    EdbSegP* trackseg;


    // Make a new PVRec object anyway
    EdbPVRec* eAli_target = new EdbPVRec();
    eAli_target->Print();

    Bool_t seg_in_eTracks=kFALSE;

    for (int i = 0; i <eAli_target->Npatterns(); i++ ) {
        if (gEDBDEBUGLEVEL>2) cout << "Looping over eAli_target->Pat()=" << i << endl;

        EdbPattern* pat = eAli_target->GetPattern(i);
        EdbPattern* pt= new EdbPattern();
        // SetPattern Values to the parent patterns:
        pt->SetID(pat->ID());
        pt->SetPID(pat->PID());
        pt->SetZ(pat->Z());

        for (int j = 0; j <pat->N(); j++ ) {
// 		for (int j = 0; j <10; j++ ) {
            EdbSegP* seg = pat->GetSegment(j);
            seg->PrintNice();

            if (gEDBDEBUGLEVEL>3) cout << "Looping over eTracks for segment nr=" << j << endl;
            for (int ll = 0; ll<TracksN; ll++ ) {
                track=(EdbTrackP*)Tracks->At(ll);
                //track->PrintNice();
                Int_t TrackSegN=track->N();

                // Here decide f.e. which tracks to check...
                // On cosmics it would be nice that f.e. Npl()>=Npatterns-4 ...
                // if ....()....
                // Since addresses of objects can vary, its better to compare them
                // by absolute positions.

                for (int kk = 0; kk<TrackSegN; kk++ ) {
                    trackseg=track->GetSegment(kk);
                    if (TMath::Abs(seg->Z()-trackseg->Z())>10.1) continue;
                    if (TMath::Abs(seg->X()-trackseg->X())>5.1) continue;
                    if (TMath::Abs(seg->Y()-trackseg->Y())>5.1) continue;
                    if (TMath::Abs(seg->TX()-trackseg->TX())>0.05) continue;
                    if (TMath::Abs(seg->TY()-trackseg->TY())>0.05) continue;
                    //cout << "Found compatible segment!! " << endl;
                    seg_in_eTracks=kTRUE;
                }
                if (seg_in_eTracks) break;
            }
            if (seg_in_eTracks) continue;
            seg_in_eTracks=kFALSE;

            // Add segment:
            if (gEDBDEBUGLEVEL>3) cout << "// Add segment:" << endl;
            pt->AddSegment(*seg);
        }
        if (gEDBDEBUGLEVEL>2) cout << "// Add AddPattern:" << endl;
        eAli_target->AddPattern(pt);
    }

    if (gEDBDEBUGLEVEL>2) eAli_source->Print();
    if (gEDBDEBUGLEVEL>2) eAli_target->Print();

    cout << "-----     void EdbPVRQuality::Remove_Passing()...done." << endl;
    return eAli_target;
}

//___________________________________________________________________________________

void EdbPVRQuality::Remove_TrackArray(TObjArray* trackArray) {

    /// Quick and Dirty implementations !!!
    // track array
    EdbPVRec* eAli_source=NULL;
    EdbPVRec* aliSource=NULL;

    if (NULL==aliSource) {
        cout << "-----     void EdbPVRQuality::Source EdbPVRec is NULL. Change to object eAli_orig: " << eAli_orig << endl;
        eAli_source=eAli_orig;
    }

    if (NULL==eAli_orig) {
        cout << "-----     void EdbPVRQuality::Also eAli_orig EdbPVRec is NULL. Do nothing and return NULL pointer!" << endl;
        return;
    }

    TObjArray* Tracks=trackArray;
    Int_t TracksN=trackArray->GetEntries();
    EdbTrackP* track;
    EdbSegP* trackseg;


    // Make a new PVRec object anyway
    EdbPVRec* eAli_target = new EdbPVRec();
    eAli_target->Print();

    Bool_t seg_in_eTracks=kFALSE;

    for (int i = 0; i <eAli_target->Npatterns(); i++ ) {
        if (gEDBDEBUGLEVEL>2) cout << "Looping over eAli_target->Pat()=" << i << endl;

        EdbPattern* pat = eAli_target->GetPattern(i);
        EdbPattern* pt= new EdbPattern();
        // SetPattern Values to the parent patterns:
        pt->SetID(pat->ID());
        pt->SetPID(pat->PID());
        pt->SetZ(pat->Z());

        for (int j = 0; j <pat->N(); j++ ) {
            EdbSegP* seg = pat->GetSegment(j);
            seg->PrintNice();

            if (gEDBDEBUGLEVEL>3) cout << "Looping over eTracks for segment nr=" << j << endl;
            for (int ll = 0; ll<TracksN; ll++ ) {
                track=(EdbTrackP*)Tracks->At(ll);
                //track->PrintNice();
                Int_t TrackSegN=track->N();

                // Here decide f.e. which tracks to check...
                // Since addresses of objects can vary, its better to compare them
                // by absolute positions.

                for (int kk = 0; kk<TrackSegN; kk++ ) {
                    trackseg=track->GetSegment(kk);
                    if (TMath::Abs(seg->Z()-trackseg->Z())>10.1) continue;
                    if (TMath::Abs(seg->X()-trackseg->X())>5.1) continue;
                    if (TMath::Abs(seg->Y()-trackseg->Y())>5.1) continue;
                    if (TMath::Abs(seg->TX()-trackseg->TX())>0.05) continue;
                    if (TMath::Abs(seg->TY()-trackseg->TY())>0.05) continue;
                    //cout << "Found compatible segment!! " << endl;
                    seg_in_eTracks=kTRUE;
                }
                if (seg_in_eTracks) break;
            }
            if (seg_in_eTracks) continue;
            seg_in_eTracks=kFALSE;

            // Add segment:
            if (gEDBDEBUGLEVEL>3) cout << "// Add segment:" << endl;
            pt->AddSegment(*seg);
        }
        if (gEDBDEBUGLEVEL>2) cout << "// Add AddPattern:" << endl;
        eAli_modified->AddPattern(pt);
    }

    if (gEDBDEBUGLEVEL>2) eAli_source->Print();
    if (gEDBDEBUGLEVEL>2) eAli_target->Print();

    cout << "-----     void EdbPVRQuality::Remove_Passing()...done." << endl;
    return;
}

//___________________________________________________________________________________


//___________________________________________________________________________________

void EdbPVRQuality::Remove_SegmentArray(TObjArray* segArray) {

    /// Quick and Dirty implementations !!!
    // track array
    EdbPVRec* eAli_source=NULL;
    EdbPVRec* aliSource=NULL;

    if (NULL==aliSource) {
        cout << "-----     void EdbPVRQuality::Source EdbPVRec is NULL. Change to object eAli_orig: " << eAli_orig << endl;
        eAli_source=eAli_orig;
    }

    if (NULL==eAli_orig) {
        cout << "-----     void EdbPVRQuality::Also eAli_orig EdbPVRec is NULL. Do nothing and return NULL pointer!" << endl;
        return;
    }

    TObjArray* Tracks=segArray;
    Int_t TracksN=segArray->GetEntries();
    EdbSegP* trackseg;

    // Make a new PVRec object anyway
    EdbPVRec* eAli_target = new EdbPVRec();
    eAli_target->Print();

    Bool_t seg_in_eTracks=kFALSE;

    for (int i = 0; i <eAli_target->Npatterns(); i++ ) {
        if (gEDBDEBUGLEVEL>2) cout << "Looping over eAli_target->Pat()=" << i << endl;

        EdbPattern* pat = eAli_target->GetPattern(i);
        EdbPattern* pt= new EdbPattern();
        // SetPattern Values to the parent patterns:
        pt->SetID(pat->ID());
        pt->SetPID(pat->PID());
        pt->SetZ(pat->Z());

        for (int j = 0; j <pat->N(); j++ ) {
            EdbSegP* seg = pat->GetSegment(j);
            seg->PrintNice();

            if (gEDBDEBUGLEVEL>3) cout << "Looping over eTracks for segment nr=" << j << endl;
            for (int ll = 0; ll<TracksN; ll++ ) {
                trackseg=(EdbSegP*)Tracks->At(ll);
                // Here decide f.e. which EdbSegP to check...
                // Since addresses of objects can vary, its better to compare them
                // by absolute positions.
                if (TMath::Abs(seg->Z()-trackseg->Z())>10.1) continue;
                if (TMath::Abs(seg->X()-trackseg->X())>5.1) continue;
                if (TMath::Abs(seg->Y()-trackseg->Y())>5.1) continue;
                if (TMath::Abs(seg->TX()-trackseg->TX())>0.05) continue;
                if (TMath::Abs(seg->TY()-trackseg->TY())>0.05) continue;
                //cout << "Found compatible segment!! " << endl;
                seg_in_eTracks=kTRUE;
            }
            if (seg_in_eTracks) continue;
            seg_in_eTracks=kFALSE;

            // Add segment:
            if (gEDBDEBUGLEVEL>3) cout << "// Add segment:" << endl;
            pt->AddSegment(*seg);
        }
        if (gEDBDEBUGLEVEL>2) cout << "// Add AddPattern:" << endl;
        eAli_modified->AddPattern(pt);
    }

    if (gEDBDEBUGLEVEL>2) eAli_source->Print();
    if (gEDBDEBUGLEVEL>2) eAli_target->Print();

    cout << "-----     void EdbPVRQuality::Remove_Passing()...done." << endl;
    return;
}

//___________________________________________________________________________________


void EdbPVRQuality::Remove_Track(EdbTrackP* track) {
    TObjArray* trackArray= new TObjArray();
    trackArray->Add(track);
    Remove_TrackArray(trackArray);
    delete trackArray;
}

//___________________________________________________________________________________

void EdbPVRQuality::Remove_Segment(EdbSegP* seg) {
    TObjArray* segArray= new TObjArray();
    segArray->Add(seg);
    Remove_SegmentArray(segArray);
    delete segArray;
}
/*******************************************************************************************************
 ReadFramework is the basis for modules developed at CVL/TU Wien for the EU project READ. 
  
 Copyright (C) 2016 Markus Diem <diem@cvl.tuwien.ac.at>
 Copyright (C) 2016 Stefan Fiel <fiel@cvl.tuwien.ac.at>
 Copyright (C) 2016 Florian Kleber <kleber@cvl.tuwien.ac.at>

 This file is part of ReadFramework.

 ReadFramework is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 ReadFramework is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 The READ project  has  received  funding  from  the European  Union’s  Horizon  2020  
 research  and innovation programme under grant agreement No 674943
 
 related links:
 [1] https://cvl.tuwien.ac.at/
 [2] https://transkribus.eu/Transkribus/
 [3] https://github.com/TUWien/
 [4] https://nomacs.org
 *******************************************************************************************************/

#include "WriterDatabase.h"
#include "WriterRetrieval.h"
#include "Image.h"
#include "Utils.h"

#include <iostream>
#include <fstream>

#pragma warning(push, 0)	// no warnings from includes
// Qt Includes
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <opencv2/ml.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/imgproc/imgproc_c.h>
#pragma warning(pop)

namespace rdf {
	/// <summary>
	/// Initializes a new instance of the <see cref="WIDatabase"/> class.
	/// </summary>
	WriterDatabase::WriterDatabase() {
		// do nothing
	}
	/// <summary>
	/// Adds a file with local features representing one image to the database
	/// </summary>
	/// <param name="filePath">The file path.</param>
	void WriterDatabase::addFile(const QString filePath) {
		mInfo << "adding file " << filePath;
		cv::Mat descriptors;
		QVector<cv::KeyPoint> kp;
		loadFeatures(filePath, descriptors, kp);

		mDescriptors.append(descriptors);
		mKeyPoints.append(kp);
		mInfo << "lenght of keypoint vector:" << mKeyPoints.length();

		//QString descFile = filePath;
		//writeMatToFile(descriptors, descFile.append("-desc.txt"));
	}

	void WriterDatabase::addFile(WriterImage wi) {
		wi.calculateFeatures();
		wi.filterKeyPoints(mVocabulary.minimumSIFTSize(), mVocabulary.maximumSIFTSize());

		mDescriptors.append(wi.descriptors());
		mKeyPoints.append(wi.keyPoints());
	}

	/// <summary>
	/// Generates the vocabulary according to the type set in the vocabulary variable. If the number of PCA components is larger than 0 a PCA is applied beforehand.
	/// </summary>
	void WriterDatabase::generateVocabulary() {
		if(mVocabulary.type() == WriterVocabulary::WI_UNDEFINED || mVocabulary.numberOfCluster() <= 0 ) {
			mWarning << " WIDatabase: vocabulary type and number of clusters have to be set before generating a new vocabulary";
			return;
		}
		if(mDescriptors.size() == 0) {
			mWarning << " WIDatabase: at least one image has to be in the dataset before generating a new vocabulary";
			return;
		}
		mInfo << "generating vocabulary:" << mVocabulary.toString();

		rdf::Image::imageInfo(mDescriptors[0], "mDescriptors[0]");
		cv::Mat allDesc(0, 0, CV_32FC1);
		for(int i = 0; i < mDescriptors.size(); i++) {
			allDesc.push_back(mDescriptors[i]);
		}

		int maxDescs = 1000000;
		if(allDesc.rows > maxDescs) {
			mInfo << "currently " << allDesc.rows << " descriptors ... reducing it to approx. " << maxDescs;
			cv::Mat tmpDesc(0, 0, CV_32FC1);
			int stepSize = allDesc.rows / maxDescs;
			if(stepSize < 1)
				stepSize = 1;
			for(int i = 0; i < allDesc.rows; i += stepSize)
				tmpDesc.push_back(allDesc.row(i).clone());
			allDesc = tmpDesc;
			mInfo << "successfully reduced number of descriptors to " << allDesc.rows;
		}

		if(mVocabulary.numberOfPCA() > 0) { 
			rdf::Image::imageInfo(allDesc, "allDesc vor aufruf calculatePCA");
			allDesc = calculatePCA(allDesc, mVocabulary.l2before()); 
		}		

		switch(mVocabulary.type()) {
		case WriterVocabulary::WI_BOW:	generateBOW(allDesc); break;
		case WriterVocabulary::WI_GMM:	generateGMM(allDesc); break;
		default: qWarning() << "WIVocabulary has unknown type"; // should not happen
			return;
		}

		// allDesc.cols is either PCA number or descriptor size
		mInfo << "mVocabulary.numberOfCluster() * allDesc.cols:" << mVocabulary.numberOfCluster() * allDesc.cols;
		cv::Mat allHists = cv::Mat(0, mVocabulary.numberOfCluster() * allDesc.cols, CV_32F);
		mInfo << "calculating histograms for all images";
		for(int i = 0; i < mDescriptors.length(); i++) {
			cv::Mat hist = mVocabulary.generateHist(mDescriptors[i]);
			allHists.push_back(hist);
		}
		
		if(mVocabulary.numberOfPCAWhiteningComponents() > 0) {
			mInfo << "generating PCA whitening";
			cv::PCA pca = cv::PCA(allHists, cv::Mat(), CV_PCA_DATA_AS_ROW, mVocabulary.numberOfPCAWhiteningComponents());
			mVocabulary.setPcaWhiteEigenvectors(pca.eigenvectors);
			mVocabulary.setPcaWhiteEigenvalues(pca.eigenvalues);
			mVocabulary.setPcaWhiteMean(pca.mean);
		}
		else {
			//calculate mean and stddev for L2 normalization
			cv::Mat means, stddev;
			for(int i = 0; i < allHists.cols; i++) {
				cv::Scalar m, s;
				meanStdDev(allHists.col(i), m, s);
				means.push_back(m.val[0]);
				stddev.push_back(s.val[0]);
			}
			stddev.convertTo(stddev, CV_32F);
			means.convertTo(means, CV_32F);
			mVocabulary.setHistL2Mean(means);
			mVocabulary.setHistL2Sigma(stddev);
		}
	}
	/// <summary>
	/// Sets the vocabulary for this database
	/// </summary>
	/// <param name="voc">The voc.</param>
	void WriterDatabase::setVocabulary(const WriterVocabulary voc) {
		mVocabulary = voc;
	}
	/// <summary>
	/// returns the current vocabulary
	/// </summary>
	/// <returns>the current vocabulary</returns>
	WriterVocabulary WriterDatabase::vocabulary() const {
		return mVocabulary;
	}
	/// <summary>
	/// Calls the saveVocabulary function of the current vocabulary
	/// </summary>
	/// <param name="filePath">The file path.</param>
	void WriterDatabase::saveVocabulary(QString filePath) {
		mVocabulary.saveVocabulary(filePath);
	}
	/// <summary>
	/// Evaluates the database.
	/// </summary>
	/// <param name="classLabels">The class labels.</param>
	/// <param name="filePaths">The files paths of the images if needed in the evaluation output</param>
	/// <param name="evalFilePath">If set a csv file with the evaluation is written to the path.</param>
	void WriterDatabase::evaluateDatabase(QStringList classLabels, QStringList filePaths, QString evalFilePath)  {
		mInfo << "evaluating database";
		if(mVocabulary.histL2Mean().empty())
			mInfo << "no l2 normalization of the histogram";
		if(std::abs(mVocabulary.powerNormalization() - 1.0f) > DBL_EPSILON)
			mInfo << "power normalization of " << mVocabulary.powerNormalization() << " applied to the feature vector";


		if(mDescriptors.empty() && !filePaths.empty()) { // load features if not already loaded
			mInfo << "descriptors empty, loading features from filePaths";
			for(int i = 0; i < filePaths.length(); i++) {
				cv::Mat desc;
				QVector<cv::KeyPoint> kp;
				loadFeatures(QString(filePaths.at(i)), desc, kp);
				mDescriptors.append(desc);
				mKeyPoints.append(kp);
			}
		}

		cv::Mat hists;
		mInfo << "calculating histograms for all images";
		for(int i = 0; i < mDescriptors.length(); i++) {
			hists.push_back(mVocabulary.generateHist(mDescriptors[i]));
		}
		evaluateDatabase(hists, classLabels, filePaths, evalFilePath);
	}

	/// <summary>
	/// Evaluates the database with the histograms stored in the vector hists
	/// </summary>
	/// <param name="hists">A vector of the histograms.</param>
	/// <param name="classLabels">The class labels.</param>
	/// <param name="filePaths">The file paths.</param>
	/// <param name="evalFilePath">The eval file path.</param>
	void WriterDatabase::evaluateDatabase(cv::Mat hists, QStringList classLabels, QStringList filePaths, QString evalFilePath) const {
		if(classLabels.empty()) {
			mWarning << "unable to evaluate database without classLabels";
			return;
		}
		mInfo << "starting evaluation";
		int tp = 0; 
		int fp = 0;
		QVector<int> soft;
		QVector<int> hard;
		for(int i = 0; i <= 10; i++) {
			soft.push_back(0);
			hard.push_back(0);
		}

		
		cv::Mat avgPrec(0, 0, CV_32F);
		cv::Mat dist = mVocabulary.calcualteDistanceMatrix(hists);
		for(int i = 0; i < hists.rows; i++) {
			cv::Mat distances = dist.row(i).t();
			//cv::Mat distances(hists.rows, 1, CV_32FC1);
			//distances.setTo(0);

			//for(int j = 0; j < hists.length(); j++) {
			//	if(mVocabulary.type() == WriterVocabulary::WI_GMM) {
			//		distances.at<float>(j) = (float)(1 - hists[i].dot(hists[j]) / (cv::norm(hists[i])*cv::norm(hists[j]) + DBL_EPSILON)); // 1-dist ... 0 is equal 2 is orthogonal
			//	} else if(mVocabulary.type() == WriterVocabulary::WI_BOW) {
			//		cv::Mat tmp;
			//		pow(hists[i] - hists[j], 2, tmp);
			//		cv::Scalar scal = cv::sum(tmp);
			//		distances.at<float>(j) = (float)sqrt(scal[0]);
			//	}
			//}
			cv::Mat idxs;
			//writeMatToFile(distances, "c:\\tmp\\distances-unsorted.txt");
			cv::sortIdx(distances, idxs, CV_SORT_EVERY_COLUMN| CV_SORT_ASCENDING);
			cv::sort(distances, distances, CV_SORT_EVERY_COLUMN | CV_SORT_ASCENDING);

			//writeMatToFile(distances, "c:\\tmp\\distances.txt");
			//writeMatToFile(idxs, "c:\\tmp\\distances-idxs.txt");
			//QFile file("c:\\tmp\\distances-idxs.txt");
			//file.open(QIODevice::ReadWrite);
			//QTextStream stream(&file);
			//QString out;
			//for(int i = 0; i < idxs.rows; i++) {
			//	out += QString::number(idxs.at<int>(i)) + "\n";
			//}
			//stream << out;
			//file.close();

			//QFile file2(filePaths[i].append("-hist.txt"));
			//file2.open(QIODevice::ReadWrite | QIODevice::Truncate);
			//QTextStream stream2(&file2);
			//QString out2;
			//for(int j = 0; j < hists[i].cols; j++) {
			//	out2 += QString::number(hists[i].at<float>(j));
			//	out2 += " ";
			//}
			//stream2 << out2 << "\n";
			//file2.close();


			if(!evalFilePath.isEmpty()) {
				QFile file(evalFilePath);
				if(file.open(QIODevice::ReadWrite | QIODevice::Append)) {
					QTextStream stream(&file);
					QFileInfo fi = QFileInfo(filePaths[i]);
					// eval file: file path , real label
					stream << "'" << fi.baseName() << "',' " << fi.absoluteFilePath() << "', " << classLabels[i] << ",";
					for(int k = 0; k < idxs.rows; k++) {
						// eval file: real writer id, distance, number of page
						QString out = classLabels[idxs.at<int>(k)] + "," + QString::number(distances.at<float>(k)) + "," + QString::number(idxs.at<int>(k)) + ",";
						stream << out;
					}
					stream << "\n";
				}
				file.close();
			}
			//qDebug() << "classLabels[i].toInt():" << classLabels[i].toInt() << " idxs.at<int>(1):" << idxs.at<int>(1) << " classLabels[idxs.at<int>(1)]:" << classLabels[idxs.at<int>(1)];
			if(classLabels[i] == classLabels[idxs.at<int>(1)])
				tp++;
			else
				fp++;
			if(idxs.rows > 11) {
				bool allCorrect = true;
				bool oneCorrect = false;
				for(int j = 1; j <= 11; j++) { // 1 because idx 0 is the original file
					if(classLabels[i] == classLabels[idxs.at<int>(j)]) 
						oneCorrect = true;
					else
						allCorrect = false;

					if(oneCorrect)
						soft[j-1] += 1;
					if(allCorrect)
						hard[j-1] += 1;
				}
			}

			// calculating mean average precession
			float sum = 0;
			int pageOfWriter = 0;
			for(int j = 1; j < idxs.rows; j++) { // 1 because idx 0 is the original file
				if(classLabels[i] == classLabels[idxs.at<int>(j)]) {
					sum += (float)++pageOfWriter / j; // ++ before so that the first page is one
				}
			}
			avgPrec.push_back(sum / pageOfWriter);
		}
		
		cv::Scalar map = cv::mean(avgPrec);

		// begin evluation output
		mInfo << "total:" << tp+fp << " tp:" << tp << " fp:" << fp;
		mInfo << "precision:" << (float)tp / ((float)tp + fp);
		mInfo << "map:" << map.val[0];

		QVector<float> softPerc;
		QVector<float> hardPerc;
		for(int i = 0; i < soft.size(); i++) {
			softPerc.push_back(soft[i] / (float)(tp + fp));
			hardPerc.push_back(hard[i] / (float)(tp + fp));
		}

		QVector<int> softCriteria({ 1, 2, 5, 7, 10 });
		QString softOutputHeader = "soft evaluation\n";
		QString softOutput = "";
		for(int i = 0; i < softCriteria.size(); i++) {
			if(softCriteria[i] > soft.size()) {
				mWarning << "Database evaluation: criteria " << softCriteria[i] << " is larger than " << soft.size()-1 << " ... skipping";
				continue;
			}
			softOutputHeader += "Top " + QString::number(softCriteria[i]) + "\t";
			softOutput += QString::number(softPerc[softCriteria[i] - 1], 'f', 3) + "\t";
		}

		//QVector<int> hardCriteria({ 2, 5, 7 });
		QVector<int> hardCriteria({ 2, 3, 4 });
		QString hardOutputHeader = "hard evaluation:\n";
		QString hardOutput = "";
		for(int i = 0; i < hardCriteria.size(); i++) {
			if(hardCriteria[i] > hard.size()) {
				mWarning << "Database evaluation: criteria " << hardCriteria[i] << " is larger than " << hard.size()-1 << " ... skipping";
				continue;
			}
			hardOutputHeader += "Top " + QString::number(hardCriteria[i]) + "\t";
			hardOutput += QString::number(hardPerc[hardCriteria[i] - 1], 'f', 3) + "\t";
		}
		qDebug() << mVocabulary.toString();
		qDebug().noquote() << softOutputHeader;
		qDebug().noquote() << softOutput;
		qDebug().noquote() << hardOutputHeader;
		qDebug().noquote() << hardOutput;
		
	}
	void WriterDatabase::writeCompetitionEvaluationFile(QStringList imageNames, QString outputPath) const {
		cv::Mat hists;
		mInfo << "calculating histograms for all images";
		for(int i = 0; i < mDescriptors.length(); i++) {
			hists.push_back(mVocabulary.generateHist(mDescriptors[i]));
		}

		writeCompetitionEvaluationFile(hists, imageNames, outputPath);
	}
	void WriterDatabase::writeCompetitionEvaluationFile(cv::Mat hists, QStringList imageNames, QString outputPath) const {
		cv::Mat dist = mVocabulary.calcualteDistanceMatrix(hists);
		QString outputString;
		for(int i = 0; i < hists.rows; i++) {
			cv::Mat distances = dist.row(i).t();
			cv::Mat idxs;
			cv::sortIdx(distances, idxs, CV_SORT_EVERY_COLUMN | CV_SORT_ASCENDING);
			cv::sort(distances, distances, CV_SORT_EVERY_COLUMN | CV_SORT_ASCENDING);

			for(int j = 0; j < idxs.rows; j++) {
				if(j != 0)
					outputString += ",";
				outputString += imageNames[idxs.at<int>(j)];
			}
			outputString += "\n";
		}

		QFile file(outputPath);
		if(file.open(QIODevice::WriteOnly)) {
			QTextStream stream(&file);
			stream << outputString;
			file.close();
		}
	}
	/// <summary>
	/// Debug name.
	/// </summary>
	/// <returns></returns>
	QString WriterDatabase::debugName() const {
		return QString("WriterIdentificationDatabase");
	}
	/// <summary>
	/// Calculates a PCA according to the components set in the vocabulary.
	/// </summary>
	/// <param name="desc">The desc.</param>
	/// <param name="normalizeBefore">if true the descriptors are normalized before applying the PCA.</param>
	/// <returns>the projected descriptors</returns>
	cv::Mat WriterDatabase::calculatePCA(const cv::Mat desc, bool normalizeBefore) {
		cv::Mat descResult;
		if(normalizeBefore) {
			// calculate mean and stddev for L2 normalization
			cv::Mat means, stddev;
			for(int i = 0; i < desc.cols; i++) {
				cv::Scalar m, s;
				meanStdDev(desc.col(i), m, s);
				means.push_back(m.val[0]);
				stddev.push_back(s.val[0]);
			}
			stddev.convertTo(stddev, CV_32F);
			means.convertTo(means, CV_32F);
			mVocabulary.setL2Mean(means);
			mVocabulary.setL2Sigma(stddev);

			// L2 normalization 
			rdf::Image::imageInfo(desc, "desc vor l2");
			descResult = (desc - cv::Mat::ones(desc.rows, 1, CV_32F) * means.t());

			//cv::Mat descResult = desc.clone();
			//qDebug() << "using modified L2";

			rdf::Image::imageInfo(descResult, "descResults vor l2 after subtracting means");
			for(int i = 0; i < descResult.rows; i++) {
				descResult.row(i) = descResult.row(i) / stddev.t();	// caution - don't clone this line unless you know what you do (without an operator / the assignment does nothing)
			}
			rdf::Image::imageInfo(descResult, "descResults nach l2");
		}
		else {
			mDebug << "normalization before L2 is turned off ... skipping";
			descResult = desc;
		}
		mInfo << "calculating PCA";
		cv::PCA pca = cv::PCA(descResult, cv::Mat(), CV_PCA_DATA_AS_ROW, mVocabulary.numberOfPCA());
		mVocabulary.setPcaEigenvectors(pca.eigenvectors);
		mVocabulary.setPcaEigenvalues(pca.eigenvalues);
		mVocabulary.setPcaMean(pca.mean);
		

		descResult = mVocabulary.applyPCA(descResult);
		rdf::Image::imageInfo(descResult, "descResults nach pca");
		return descResult;
	}
	/// <summary>
	/// Generates the BagOfWords for the given descriptors.
	/// </summary>
	/// <param name="desc">The desc.</param>
	void WriterDatabase::generateBOW(cv::Mat desc) {
		cv::BOWKMeansTrainer bow(mVocabulary.numberOfCluster(), cv::TermCriteria(), 10);
		cv::Mat voc = bow.cluster(desc);
		mVocabulary.setVocabulary(voc);
	}
	/// <summary>
	/// Generates the GMMs and the Fisher information for the given descriptors.
	/// </summary>
	/// <param name="desc">The desc.</param>
	void WriterDatabase::generateGMM(cv::Mat desc) {
		mInfo << "start training GMM";		
		cv::Ptr<cv::ml::EM> em = cv::ml::EM::create();
		em->setClustersNumber(mVocabulary.numberOfCluster());
		em->setCovarianceMatrixType(cv::ml::EM::COV_MAT_DIAGONAL);
		cv::TermCriteria tc = cv::TermCriteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS, 5000, FLT_EPSILON);
		em->setTermCriteria(tc);

		mInfo << "start training GMM - number of features:" << desc.rows; 
		rdf::Image::imageInfo(desc, "all descriptors");
		if(!em->trainEM(desc)) {
			qWarning() << "unable to train GMM";
			return;
		} 
		
		mVocabulary.setEM(em);
		mInfo << "finished";
	}
	/// <summary>
	/// Writes an opencv mat to txt file.
	/// </summary>
	/// <param name="mat">mat</param>
	/// <param name="filePath">The file path.</param>
	void WriterDatabase::writeMatToFile(const cv::Mat mat, const QString filePath) const {
		std::ofstream fileStream;
		fileStream.open(filePath.toStdString());
		fileStream << mat.cols << "\n" << mat.rows << "\n" << std::flush;
		for(int i = 0; i < mat.rows; i++) {
			const float* row = mat.ptr<float>(i);
			for(int j = 0; j < mat.cols; j++)
				fileStream << row[j] << " ";
			fileStream << "\n" << std::flush;
		}
		fileStream.close();
	}

	/// <summary>
	/// Loads the features from the given file path and puts it into descriptors and keypoints
	/// </summary>
	/// <param name="filePath">The file path.</param>
	/// <param name="descriptors">The descriptors which are read from the file.</param>
	/// <param name="keypoints">The keypoints which are read from the file.</param>
	void WriterDatabase::loadFeatures(const QString filePath, cv::Mat & descriptors, QVector<cv::KeyPoint>& keypoints) {
		cv::FileStorage fs(filePath.toStdString(), cv::FileStorage::READ);
		if(!fs.isOpened()) {
			qWarning() << debugName() << " unable to read file " << filePath;
			return;
		}
		std::vector<cv::KeyPoint> kp;
		fs["keypoints"] >> kp;
		fs["descriptors"] >> descriptors;
		fs.release();
		QVector<cv::KeyPoint> kpQt = QVector<cv::KeyPoint>::fromStdVector(kp);

		if(mVocabulary.minimumSIFTSize() > 0 || mVocabulary.maximumSIFTSize() > 0) {

			WriterImage wi = WriterImage();
			wi.setKeyPoints(kpQt);
			wi.setDescriptors(descriptors);

			wi.filterKeyPoints(mVocabulary.minimumSIFTSize(), mVocabulary.maximumSIFTSize());
			kpQt = wi.keyPoints();
			descriptors = wi.descriptors();
		}
		else
			mInfo << "not filtering SIFT features, vocabulary is emtpy, or min or max size not set";

		keypoints = kpQt;
	}

	// WIVocabulary ----------------------------------------------------------------------------------
	/// <summary>
	/// Initializes a new instance of the <see cref="WIVocabulary"/> class.
	/// </summary>
	WriterVocabulary::WriterVocabulary() {
		// do nothing
	}
	/// <summary>
	/// Loads the vocabulary from the given file path.
	/// updates the mVocabulary path
	/// </summary>
	/// <param name="filePath">The file path.</param>
	void WriterVocabulary::loadVocabulary(const QString filePath) {
		mInfo << "loading vocabulary from " << filePath; 
		cv::FileStorage fs(filePath.toStdString(), cv::FileStorage::READ);
		if(!fs.isOpened()) {
			mWarning << "WIVocabulary: unable to read file " << filePath;
			return;
		}
		fs["PcaMean"] >> mPcaMean;
		fs["PcaEigenvectors"] >> mPcaEigenvectors;
		fs["PcaEigenvalues"] >> mPcaEigenvalues;
		fs["PcaWhiteMean"] >> mPcaWhiteMean;
		fs["PcaWhiteEigenvectors"] >> mPcaWhiteEigenvectors;
		fs["PcaWhiteEigenvalues"] >> mPcaWhiteEigenvalues;
		fs["L2Mean"] >> mL2Mean;
		fs["L2Sigma"] >> mL2Sigma;
		fs["histL2Mean"] >> mHistL2Mean;
		fs["histL2Sigma"] >> mHistL2Sigma;
		fs["NumberOfClusters"] >> mNumberOfClusters;
		fs["NumberOfPCA"] >> mNumberPCA;
		fs["NumberOfPCAWhitening"] >> mNumPCAWhiteComponents;
		fs["type"] >> mType;
		fs["minimumSIFTSize"] >> mMinimumSIFTSize;
		fs["maximumSIFTSize"] >> mMaximumSIFTSize;
		fs["powerNormalization"] >> mPowerNormalization;
		fs["L2before"] >> mL2Before;
		std::string note;
		fs["note"] >> note;
		mNote = QString::fromStdString(note);
		if(mType == WI_BOW)
			fs["Vocabulary"] >> mVocabulary;
		else {
			std::string gmmPath;
			fs["GmmPath"] >> gmmPath;

			if (QFileInfo(QString::fromStdString(gmmPath)).exists()) {
// Dear future me: we peaked before 3.2, there we needed this ifdef
// so I leave it here in case this line fails again
#if RDF_OPENCV_VERSION > RDF_VERSION(3,2,0)
				mEM = cv::ml::EM::load(gmmPath);
#else
				mEM = cv::ml::EM::load<cv::ml::EM>(gmmPath);
#endif
			}
			else
				mWarning << "gmm file " << QString::fromStdString(gmmPath) << " (stored in the vocabulary) not found!";

			//cv::FileNode fn = fs["StatModel.EM"];
			//mEM->read(fn);
		}

		fs.release();
		mVocabularyPath = filePath;
	}
	/// <summary>
	/// Saves the vocabulary to the given file path.
	/// updates the mVocabularyPath, thus this method is not const
	/// </summary>
	/// <param name="filePath">The file path.</param>
	void WriterVocabulary::saveVocabulary(const QString filePath) {
		mInfo << "saving vocabulary to " << filePath;
		if(isEmpty()) {
			mWarning << "WIVocabulary: isEmpty() is true ... unable to save to file";
			return;
		}
		cv::FileStorage fs(filePath.toStdString(), cv::FileStorage::WRITE);

		fs << "description" << toString().toStdString();
		fs << "NumberOfClusters" << mNumberOfClusters;
		fs << "NumberOfPCA" << mNumberPCA;
		fs << "NumberOfPCAWhitening" << mNumPCAWhiteComponents;
		fs << "type" << mType;
		fs << "powerNormalization" << mPowerNormalization;
		fs << "minimumSIFTSize" << mMinimumSIFTSize;
		fs << "maximumSIFTSize" << mMaximumSIFTSize;
		//fs << "note" << mNote.toStdString();
		fs << "PcaMean" << mPcaMean;
		fs << "PcaEigenvectors" << mPcaEigenvectors;
		fs << "PcaEigenvalues" << mPcaEigenvalues;
		fs << "PcaWhiteMean" << mPcaWhiteMean;
		fs << "PcaWhiteEigenvectors" << mPcaWhiteEigenvectors;
		fs << "PcaWhiteEigenvalues" << mPcaWhiteEigenvalues;
		fs << "L2Mean" << mL2Mean;
		fs << "L2Sigma" << mL2Sigma;
		fs << "histL2Mean" << mHistL2Mean;
		fs << "histL2Sigma" << mHistL2Sigma;
		fs << "L2before" << mL2Before;
		if(mType == WI_BOW)
			fs << "Vocabulary" << mVocabulary;
		else if(mType == WI_GMM) {
			QString gmmPath = filePath;
			gmmPath.insert(gmmPath.length() - 4, "-gmm");
			fs << "GmmPath" << gmmPath.toStdString();
			mEM->save(gmmPath.toStdString());
			
			//mEM->write(fs);
		}
		mVocabularyPath = filePath;
		fs.release();
	}
	/// <summary>
	/// Calcualtes the distance matrix of the histograms given in the RxC matrix. Depending on the type of the vocabulary either
	/// the consine distance (GMM) or the euclidean (BOW) distance is used.
	/// </summary>
	/// <param name="hists">a matrix with the feature vectors of different images stored in the rows</param>
	/// <returns>a RxR matrix of the distances between the feature vectors in the rows of the input matrix.</returns>
	cv::Mat WriterVocabulary::calcualteDistanceMatrix(cv::Mat hists) const {
		cv::Mat distances = cv::Mat(hists.rows, hists.rows, CV_32F);
		distances.setTo(0);
		for(int i = 0; i < hists.rows; i++) {
			for(int j = i; j < hists.rows; j++) {
				if(mVocabulary.type() == WriterVocabulary::WI_GMM) {
					distances.at<float>(i, j) = (float)(1 - hists.row(i).dot(hists.row(j)) / (cv::norm(hists.row(i))*cv::norm(hists.row(j)) + DBL_EPSILON)); // 1-dist ... 0 is equal 2 is orthogonal
					distances.at<float>(j, i) = distances.at<float>(i, j);
				}
				else if(mVocabulary.type() == WriterVocabulary::WI_BOW) {
					cv::Mat tmp;
					pow(hists.row(i) - hists.row(j), 2, tmp);
					cv::Scalar scal = cv::sum(tmp);
					distances.at<float>(i,j) = (float)sqrt(scal[0]);
					distances.at<float>(j, i) = distances.at<float>(i, j);
				}
			}
		}
		return distances;
	}


	/// <summary>
	/// Determines whether the vocabulary is empty respl. not trained.
	/// </summary>
	/// <returns></returns>
	bool WriterVocabulary::isEmpty() const {
		
		if ((mVocabulary.empty() && mType == WI_BOW) || 
			(mEM.empty() && mType == WI_GMM) || 
			mNumberOfClusters <= 0 || mType == WI_UNDEFINED) {
			return true;
		}
		return false;
	}
	/// <summary>
	/// Sets the vocabulary for BOW.
	/// </summary>
	/// <param name="voc">The voc.</param>
	void WriterVocabulary::setVocabulary(cv::Mat voc) {
		mVocabulary = voc;
	}
	/// <summary>
	/// BOW vocabulary of this instance
	/// </summary>
	/// <returns>the BOW vocabulary</returns>
	cv::Mat WriterVocabulary::vocabulary() const {
		return mVocabulary;
	}
	/// <summary>
	/// Sets the em for GMM.
	/// </summary>
	/// <param name="em">The em.</param>
	void WriterVocabulary::setEM(cv::Ptr<cv::ml::EM> em) {
		mEM = em;
	}
	/// <summary>
	/// the EM of this instance.
	/// </summary>
	/// <returns></returns>
	cv::Ptr<cv::ml::EM> WriterVocabulary::em() const {
		return mEM;
	}
	/// <summary>
	/// Sets the mean Mat of the PCA.
	/// </summary>
	/// <param name="mean">The mean Mat.</param>
	void WriterVocabulary::setPcaMean(const cv::Mat mean) {
		mPcaMean = mean;
	}
	/// <summary>
	/// Mean values of the PCA.
	/// </summary>
	/// <returns>mean Mat</returns>
	cv::Mat WriterVocabulary::pcaMean() const {
		return mPcaMean;
	}
	/// <summary>
	/// Sets the pca eigenvectors.
	/// </summary>
	/// <param name="ev">The eigenvectors</param>
	void WriterVocabulary::setPcaEigenvectors(const cv::Mat ev) {
		mPcaEigenvectors = ev;
	}
	/// <summary>
	/// Returns the Eigenvectors of the PCA.
	/// </summary>
	/// <returns>Mat of the Eigenvectors</returns>
	cv::Mat WriterVocabulary::pcaEigenvectors() const {
		return mPcaEigenvectors;
	}
	/// <summary>
	/// Sets the PCA eigenvalues
	/// </summary>
	/// <param name="ev">The eigenvalues</param>
	void WriterVocabulary::setPcaEigenvalues(const cv::Mat ev) {
		mPcaEigenvalues = ev;
	}
	/// <summary>
	/// Returns the Eigenvalues of the PCA
	/// </summary>
	/// <returns>Mat of the eigenvalues</returns>
	cv::Mat WriterVocabulary::pcaEigenvalues() const {
		return mPcaEigenvalues;
	}
	void WriterVocabulary::setPcaWhiteMean(cv::Mat mean) {
		mPcaWhiteMean = mean;
	}
	cv::Mat WriterVocabulary::pcaWhiteMean() const {
		return mPcaWhiteMean;
	}
	void WriterVocabulary::setPcaWhiteEigenvectors(cv::Mat ev) {
		mPcaWhiteEigenvectors = ev;
	}
	cv::Mat WriterVocabulary::pcaWhiteEigenvectors() const {
		return mPcaWhiteEigenvectors;
	}
	void WriterVocabulary::setPcaWhiteEigenvalues(cv::Mat ev) {
		mPcaWhiteEigenvalues = ev;
	}
	cv::Mat WriterVocabulary::pcaWhiteEigenvalues() const {
		return mPcaEigenvalues;
	}
	/// <summary>
	/// sets the mean Mat of the L2 noramlization
	/// </summary>
	/// <param name="l2mean">Mean Mat.</param>
	void WriterVocabulary::setL2Mean(const cv::Mat l2mean) {
		mL2Mean = l2mean;
	}
	/// <summary>
	/// Returns the mean Mat of the L2 normalization
	/// </summary>
	/// <returns>mean Mat</returns>
	cv::Mat WriterVocabulary::l2Mean() const {
		return mL2Mean;
	}
	/// <summary>
	/// Sets the variance of the L2 normalization.
	/// </summary>
	/// <param name="l2sigma">variance Mat.</param>
	void WriterVocabulary::setL2Sigma(const cv::Mat l2sigma) {
		mL2Sigma = l2sigma;
	}
	/// <summary>
	/// Returns the variance Mat of the L2 normalization
	/// </summary>
	/// <returns>variance Mat </returns>
	cv::Mat WriterVocabulary::l2Sigma() const {
		return mL2Sigma;
	}
	/// <summary>
	/// Sets the l2 mean which is applied to the histograms.
	/// </summary>
	/// <param name="mean">The mean.</param>
	void WriterVocabulary::setHistL2Mean(const cv::Mat mean) {
		mHistL2Mean = mean;
	}
	/// <summary>
	/// Returns the means which are applied to the histogram
	/// </summary>
	/// <returns></returns>
	cv::Mat WriterVocabulary::histL2Mean() const {
		return mHistL2Mean;
	}
	/// <summary>
	/// Sets the l2 mean which is applied to the features.
	/// </summary>
	/// <param name="sigma">The sigma.</param>
	void WriterVocabulary::setHistL2Sigma(const cv::Mat sigma) {
		mHistL2Sigma = sigma;
	}
	/// <summary>
	/// Returns the means which are applied to the features
	/// </summary>
	/// <returns></returns>
	cv::Mat WriterVocabulary::histL2Sigma() const {
		return mHistL2Sigma;
	}
	/// <summary>
	/// Sets the number of cluster.
	/// </summary>
	/// <param name="number">number of clusters.</param>
	void WriterVocabulary::setNumberOfCluster(const int number) {
		mNumberOfClusters = number;
	}
	/// <summary>
	/// Numbers of clusters.
	/// </summary>
	/// <returns>number of clusters</returns>
	int WriterVocabulary::numberOfCluster() const {
		return mNumberOfClusters;
	}
	/// <summary>
	/// Sets the number of PCA components which should be used
	/// </summary>
	/// <param name="number">The number of PCA components.</param>
	void WriterVocabulary::setNumberOfPCA(const int number) {
		mNumberPCA = number;
	}
	/// <summary>
	/// Numbers the of pca.
	/// </summary>
	/// <returns>The number of PCA components</returns>
	int WriterVocabulary::numberOfPCA() const {
		return mNumberPCA;
	}
	/// <summary>
	/// Sets the type of the vocabulary.
	/// </summary>
	/// <param name="type">The type.</param>
	void WriterVocabulary::setType(const int type) {
		mType = type;
	}
	/// <summary>
	/// Returns the vocabulary type.
	/// </summary>
	/// <returns>type of the vocabulary</returns>
	int WriterVocabulary::type() const {
		return mType;
	}
	/// <summary>
	/// Sets a note to the vocabulary. 
	/// </summary>
	/// <param name="note">note.</param>
	void WriterVocabulary::setNote(QString note) {
		mNote = note;
	}
	/// <summary>
	/// Sets the minimum size for sift features, all features smaller than this size are filtered out.
	/// </summary>
	/// <param name="size">The minimum size in pixels.</param>
	void WriterVocabulary::setMinimumSIFTSize(const int size) {
		mMinimumSIFTSize = size;
	}
	/// <summary>
	/// Returns the value of the minimum SIFT features size
	/// </summary>
	/// <returns>minimum size of the SIFT features</returns>
	int WriterVocabulary::minimumSIFTSize() const {
		return mMinimumSIFTSize;
	}
	/// <summary>
	/// Sets the maximum size for sift features, all features larger than this size are filtered out.
	/// </summary>
	/// <param name="size">The size.</param>
	void WriterVocabulary::setMaximumSIFTSize(const int size) {
		mMaximumSIFTSize = size;
	}
	/// <summary>
	/// Returns the value of the maximum SIFT features size
	/// </summary>
	/// <returns>maximum size of the SIFT features</returns>
	int WriterVocabulary::maximumSIFTSize() const {
		return mMaximumSIFTSize;
	}
	/// <summary>
	/// Sets the power normalization for the feature vector.
	/// </summary>
	/// <param name="power">The power normalization factor.</param>
	void WriterVocabulary::setPowerNormalization(const double power) {
		mPowerNormalization = power;
	}
	/// <summary>
	/// Returns the factor of the power normalization used.
	/// </summary>
	/// <returns>the current power normalization factor</returns>
	double WriterVocabulary::powerNormalization() const {
		return mPowerNormalization;
	}
	void WriterVocabulary::setNumOfPCAWhiteComp(const int numOfComp) {
		mNumPCAWhiteComponents = numOfComp;
	}
	int WriterVocabulary::numberOfPCAWhiteningComponents() const {
		return mNumPCAWhiteComponents;
	}
	/// <summary>
	/// Sets the L2 before flag (perform L2 normalization before generating the fisher vector).
	/// </summary>
	/// <param name="l2before">The l2before.</param>
	void WriterVocabulary::setL2Before(const bool l2before) {
		mL2Before = l2before;
	}
	/// <summary>
	/// Setting if a L2 normalization should be performed before generating the feature vector
	/// </summary>
	/// <returns></returns>
	bool WriterVocabulary::l2before() const {
		return mL2Before;
	}
	/// <summary>
	/// Returns the note of the vocabulary.
	/// </summary>
	/// <returns></returns>
	QString WriterVocabulary::note() const {
		return mNote;
	}
	/// <summary>
	/// Creates a string with a description of the current vocabulary
	/// </summary>
	/// <returns>a short description</returns>
	QString WriterVocabulary::toString() const {
		QString description = "";
		if(type() == WI_GMM)
			description.append("GMM ");
		else if(type() == WI_BOW)
			description.append("BOW ");
		description += " clusters:" + QString::number(mNumberOfClusters) + " pca:" + QString::number(mNumberPCA);
		if(mNumPCAWhiteComponents > 0)
			description += " pca whitening components:" + QString::number(mNumPCAWhiteComponents);
		else
			description += " no pca whitening";
		return description;
	}
	/// <summary>
	/// Path of the vocabulary
	/// </summary>
	/// <returns></returns>
	QString WriterVocabulary::vocabularyPath() const {
		return mVocabularyPath;
	}

	/// <summary>
	/// Generates the histogram according to the vocabulary type
	/// </summary>
	/// <param name="desc">Descriptors of an image.</param>
	/// <returns>the generated histogram</returns>
	cv::Mat WriterVocabulary::generateHist(cv::Mat desc) const {
		if(mType == WriterVocabulary::WI_BOW)
			return generateHistBOW(desc);
		else if(mType == WriterVocabulary::WI_GMM) {
			return generateHistGMM(desc);
		}
		else {
			qWarning() << "vocabulary type is undefined... not generating histograms";
			return cv::Mat();
		}
	}

	/// <summary>
	/// Returns the debug name of the class
	/// </summary>
	/// <returns></returns>
	QString WriterVocabulary::debugName() {
		return "WriterVocabulary";
	}

	/// <summary>
	/// Generates the histogram for a BOW vocabulary. 
	/// </summary>
	/// <param name="desc">The desc.</param>
	/// <returns>the histogram</returns>
	cv::Mat WriterVocabulary::generateHistBOW(cv::Mat desc) const {
		if(isEmpty()) {
			qWarning() << "generateHistBOW: vocabulary is empty ... aborting";
			return cv::Mat();
		}
		if(desc.empty())
			return cv::Mat();

		cv::Mat d = desc.clone();
		if(!mL2Mean.empty())
			d = l2Norm(d, mL2Mean, mL2Sigma);

		if(numberOfPCA() > 0) {
			d = applyPCA(d);
		}
		cv::Mat dists, idx;

		cv::flann::Index flann_index(mVocabulary, cv::flann::LinearIndexParams());
		cv::Mat hist = cv::Mat(1, (int)mVocabulary.rows, CV_32FC1);
		hist.setTo(0);

		flann_index.knnSearch(d, idx, dists, 1, cv::flann::SearchParams(64));

		float *ptrHist = hist.ptr<float>(0);
		int *ptrLabels = idx.ptr<int>(0);		//float?

		for(int i = 0; i < idx.rows; i++) {

			ptrHist[(int)*ptrLabels]++;
			ptrLabels++;
		}

		hist /= (float)d.rows;


		return hist;
	}
	/// <summary>
	/// Generates the Fisher vector for a GMM vocabulary.
	/// </summary>
	/// <param name="desc">The desc.</param>
	/// <returns>the Fisher vector</returns>
	cv::Mat WriterVocabulary::generateHistGMM(cv::Mat desc) const {
		if(isEmpty()) {
			qWarning() << "generateHistGMM: vocabulary is empty ... aborting";
			return cv::Mat();
		}
		if(desc.empty())
			return cv::Mat();


		cv::Mat d = desc.clone();
		if(!mL2Mean.empty())
			d = l2Norm(d, mL2Mean, mL2Sigma);
		else
			qDebug() << "gnerateHistGMM: mVocabulary.l2Mean() is empty ... no L2 normalization (before fisher vector) done";

		if(mNumberPCA > 0) {
			d = applyPCA(d);
		}
		cv::Mat fisher(mNumberOfClusters, d.cols, CV_32F);
		fisher.setTo(0);

		cv::Ptr<cv::ml::EM> em = mEM;
		for(int i = 0; i < d.rows; i++) {
			cv::Mat feature = d.row(i);
			cv::Mat probs, means;
			std::vector<cv::Mat> covs;

			cv::Vec2d emOut = em->predict2(feature, probs);
			probs.convertTo(probs, CV_32F);
			em->getCovs(covs);
			means = em->getMeans();
			means.convertTo(means, CV_32F);
			for(int j = 0; j < em->getClustersNumber(); j++) {
				cv::Mat cov = covs[j];
				cv::Mat diag = cov.diag(0).t();
				diag.convertTo(diag, CV_32F);
				fisher.row(j) += probs.at<float>(j) * ((feature - means.row(j)) / diag);
			}
		}
		cv::Mat weights = em->getWeights();
		weights.convertTo(weights, CV_32F);
		for(int j = 0; j < em->getClustersNumber(); j++) {
			//fisher.row(j) *= 1.0f / (d.rows* sqrt(weights.at<float>(j)) + DBL_EPSILON);
			fisher.row(j) *= 1.0f / (sqrt(weights.at<float>(j)) + DBL_EPSILON);
		}
		cv::Mat hist = fisher.reshape(0, 1);

		cv::Mat tmp;
		cv::pow(cv::abs(hist), powerNormalization(), tmp);
		
		for(int i = 0; i < hist.cols; i++) {
			if(hist.at<float>(i) < 0)
				tmp.at<float>(i) *= -1;
		}
		hist = tmp;

		//qDebug() << "normalizing histogram with cv::norm";
		//hist = hist / cv::norm(hist);

		if(!mHistL2Mean.empty())
			hist = l2Norm(hist, mHistL2Mean, mHistL2Sigma);
		if(!mPcaWhiteEigenvalues.empty()) {
			qDebug() << "applying pca whitening new";		
			hist = mPcaWhiteEigenvectors*(hist - mPcaWhiteMean).t();
			cv::Mat latent;
			cv::pow(mPcaWhiteEigenvalues + FLT_EPSILON, -0.5, latent);
			hist = cv::Mat::diag(latent)*hist;
			hist /= cv::norm(hist, cv::NORM_L1);
			hist = hist.t();
		}

		return hist;
	}
	/// <summary>
	/// Applies the PCA with the stored Eigenvalues and Eigenvectors of the vocabulary.
	/// </summary>
	/// <param name="desc">The desc.</param>
	/// <returns>the projected descriptors</returns>
	cv::Mat WriterVocabulary::applyPCA(cv::Mat desc) const {
		if(mPcaEigenvalues.empty() || mPcaEigenvectors.empty() || mPcaMean.empty()) {
			qWarning() << "applyPCA: vocabulary does not have a PCA ... not applying PCA";
			return desc;
		}
		cv::PCA pca;
		pca.eigenvalues = mPcaEigenvalues;
		pca.eigenvectors = mPcaEigenvectors;
		pca.mean = mPcaMean;
		pca.project(desc, desc);

		return desc;
	}
	/// <summary>
	/// Applies a L2 normalization with the values given
	/// </summary>
	/// <param name="desc">The desc.</param>
	/// <returns>normalized descriptors</returns>
	cv::Mat WriterVocabulary::l2Norm(cv::Mat desc, cv::Mat mean, cv::Mat sigma) const {
		// L2 - normalization
		cv::Mat d = (desc - cv::Mat::ones(desc.rows, 1, CV_32F) * mean.t());

		//qDebug() << "modified L2";
		//cv::Mat d = desc;

		for(int i = 0; i < d.rows; i++) {
			d.row(i) = d.row(i) / sigma.t();
		}
		return d;
	}

	WriterVocabularyConfig::WriterVocabularyConfig() {
		mModuleName = "WriterVocabulary";
	}

	int WriterVocabularyConfig::type() const {
		return mType;
	}

	void WriterVocabularyConfig::setType(int type) {
		mType = type;
	}

	int WriterVocabularyConfig::numberOfClusters() const  {
		return mNumberOfClusters;
	}

	void WriterVocabularyConfig::setNumberOfCluster(int num) {
		mNumberOfClusters = num;
	}

	int WriterVocabularyConfig::numberOfPCA() const {
		return mNumberOfPCA;
	}

	void WriterVocabularyConfig::setNumberOfPCA(int num) {
		mNumberOfPCA = num;
	}

	int WriterVocabularyConfig::numberOfPCAWhitening() const {
		return mNumverOfPCAWhitening;
	}

	void WriterVocabularyConfig::setNumberOfPCAWhitening(int num) {
		mNumverOfPCAWhitening = num;
	}

	int WriterVocabularyConfig::maxSIFTSize() const {
		return mMaxSIFTSize;
	}

	void WriterVocabularyConfig::setMaxSIFTSize(int maxSize) {
		mMaxSIFTSize = maxSize;
	}

	int WriterVocabularyConfig::minSIFTSize() const {
		return mMinSIFTSize;
	}

	void WriterVocabularyConfig::setMINSIFTSize(int minSize) {
		mMinSIFTSize = minSize;
	}

	double WriterVocabularyConfig::powerNormalization() const {
		return mPowerNormalization;
	}

	void WriterVocabularyConfig::setPowerNormalization(float power) {
		mPowerNormalization = power;
	}

	bool WriterVocabularyConfig::l2before() const {
		return mL2NormBefore;
	}

	void WriterVocabularyConfig::setL2Before(bool performL2) {
		mL2NormBefore = performL2;
	}

	void WriterVocabularyConfig::load(const QSettings & settings) {
		mType = settings.value("vocType", mType).toInt();
		if(mType > rdf::WriterVocabulary::WI_UNDEFINED)
			mType = rdf::WriterVocabulary::WI_UNDEFINED;
		mNumberOfClusters= settings.value("numberOfClusters", mNumberOfClusters).toInt();
		mNumberOfPCA = settings.value("numberOfPCA", mNumberOfPCA).toInt();
		mNumverOfPCAWhitening = settings.value("numberOfPCAWhitening", mNumverOfPCAWhitening).toInt();
		mMaxSIFTSize = settings.value("maxSIFTSize", mMaxSIFTSize).toInt();
		mMinSIFTSize = settings.value("minSIFTSize", mMinSIFTSize).toInt();
		mPowerNormalization = settings.value("powerNormalization", mPowerNormalization).toDouble();
		mL2NormBefore = settings.value("L2before", mL2NormBefore).toBool();
	}

	void WriterVocabularyConfig::save(QSettings & settings) const {
		settings.setValue("vocType", mType);
		settings.setValue("numberOfClusters", mNumberOfClusters);
		settings.setValue("numberOfPCA", mNumberOfPCA);
		settings.setValue("numberOfPCAWhitening", mNumverOfPCAWhitening);
		settings.setValue("maxSIFTSize", mMaxSIFTSize);
		settings.setValue("minSIFTSize", mMinSIFTSize);
		settings.setValue("powerNormalization", mPowerNormalization);
		settings.setValue("L2before", mL2NormBefore);
	}

}



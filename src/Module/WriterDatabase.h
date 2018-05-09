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
 [1] http://www.cvl.tuwien.ac.at/cvl/
 [2] https://transkribus.eu/Transkribus/
 [3] https://github.com/TUWien/
 [4] http://nomacs.org
 *******************************************************************************************************/

#pragma once

#include "BaseModule.h"

#pragma warning(push, 0)	// no warnings from includes
// Qt Includes
#include <QString>
#include <QVector>
#include <opencv2/imgproc.hpp>
#include <opencv2/ml.hpp>
#pragma warning(pop)

#ifndef DllCoreExport
#ifdef DLL_CORE_EXPORT
#define DllCoreExport Q_DECL_EXPORT
#else
#define DllCoreExport Q_DECL_IMPORT
#endif
#endif


#pragma warning(disable: 4251)	// dll interface


// Qt defines

namespace rdf {
	class WriterImage;
	class WriterVocabulary;

	class DllCoreExport WriterVocabularyConfig : public ModuleConfig {
		public:
			WriterVocabularyConfig();

			int type() const;
			void setType(int type);

			int numberOfClusters() const;
			void setNumberOfCluster(int num);

			int numberOfPCA() const;
			void setNumberOfPCA(int num);

			int numberOfPCAWhitening() const;
			void setNumberOfPCAWhitening(int num);

			int maxSIFTSize() const;
			void setMaxSIFTSize(int maxSize);

			int minSIFTSize() const;
			void setMINSIFTSize(int minSize);

			double powerNormalization() const;
			void setPowerNormalization(float power);

			bool l2before() const;
			void setL2Before(bool performL2);

			//QString toString() const override;

		protected:
			void load(const QSettings& settings) override;
			void save(QSettings& settings) const override;

		private:
			int mType = -1;
			int mNumberOfClusters = 50;
			int mNumberOfPCA = 96;
			int mNumverOfPCAWhitening = 0;
			int mMaxSIFTSize = 70;
			int mMinSIFTSize = 20;
			double mPowerNormalization = 0.5;
			bool mL2NormBefore = false;
	};

	class DllCoreExport WriterVocabulary {

	public:
		WriterVocabulary();

		void loadVocabulary(const QString filePath);
		void saveVocabulary(const QString filePath);

		cv::Mat calcualteDistanceMatrix(cv::Mat hists) const;

		bool isEmpty() const;

		enum type {
			WI_GMM,
			WI_BOW,

			WI_UNDEFINED
		};

		void setVocabulary(cv::Mat voc);
		cv::Mat vocabulary() const;
		void setEM(cv::Ptr<cv::ml::EM> em);
		cv::Ptr<cv::ml::EM> em() const;
		void setPcaMean(cv::Mat mean);
		cv::Mat pcaMean() const;
		void setPcaEigenvectors(cv::Mat ev);
		cv::Mat pcaEigenvectors() const;
		void setPcaEigenvalues(cv::Mat ev);
		cv::Mat pcaEigenvalues() const;
		void setPcaWhiteMean(cv::Mat mean);
		cv::Mat pcaWhiteMean() const;
		void setPcaWhiteEigenvectors(cv::Mat ev);
		cv::Mat pcaWhiteEigenvectors() const;
		void setPcaWhiteEigenvalues(cv::Mat ev);
		cv::Mat pcaWhiteEigenvalues() const;
		void setL2Mean(const cv::Mat l2mean);
		cv::Mat l2Mean() const;
		void setL2Sigma(const cv::Mat l2sigma);
		cv::Mat l2Sigma() const;
		void setHistL2Mean(const cv::Mat mean);
		cv::Mat histL2Mean() const;
		void setHistL2Sigma(const cv::Mat sigma);
		cv::Mat histL2Sigma() const;
		void setNumberOfCluster(const int number);
		int numberOfCluster() const;
		void setNumberOfPCA(const int number);
		int numberOfPCA() const;
		void setType(const int type);
		int type() const;
		void setNote(QString note);
		void setMinimumSIFTSize(const int size);
		int minimumSIFTSize() const;
		void setMaximumSIFTSize(const int size);
		int maximumSIFTSize() const;
		void setPowerNormalization(const double power);
		double powerNormalization() const;
		void setNumOfPCAWhiteComp(const int numOfComp);
		int numberOfPCAWhiteningComponents() const;

		void setL2Before(const bool l2before);
		bool l2before() const;

		QString note() const;
		QString toString() const;

		QString vocabularyPath() const;

		cv::Mat generateHist(cv::Mat desc) const;

		cv::Mat applyPCA(cv::Mat desc) const;

	private:
		/// <summary>
		/// Debugs the name.
		/// </summary>
		/// <returns></returns>
		QString debugName();
		cv::Mat generateHistBOW(cv::Mat desc) const;
		cv::Mat generateHistGMM(cv::Mat desc) const;
		
		cv::Mat l2Norm(cv::Mat desc, cv::Mat mean, cv::Mat sigma) const;


		cv::Mat mVocabulary = cv::Mat();
		cv::Ptr<cv::ml::EM> mEM;
		cv::Mat mPcaMean = cv::Mat();
		cv::Mat mPcaEigenvectors = cv::Mat();
		cv::Mat mPcaEigenvalues = cv::Mat();
		cv::Mat mPcaWhiteMean = cv::Mat();
		cv::Mat mPcaWhiteEigenvectors = cv::Mat();
		cv::Mat mPcaWhiteEigenvalues = cv::Mat();
		cv::Mat mL2Mean = cv::Mat();
		cv::Mat mL2Sigma = cv::Mat();
		cv::Mat mHistL2Mean = cv::Mat();
		cv::Mat mHistL2Sigma = cv::Mat();

		int mNumberOfClusters = -1;
		int mNumberPCA = -1;
		int mType = WI_UNDEFINED;
		int mMinimumSIFTSize = -1;
		int mMaximumSIFTSize = -1;
		double mPowerNormalization = 1;
		QString mNote = QString();

		QString mVocabularyPath = QString();
		bool mL2Before = false;
		int mNumPCAWhiteComponents = 0;
	};

// read defines
	class DllCoreExport WriterDatabase {

	public:
		WriterDatabase();

		void addFile(const QString filePath);
		void addFile(WriterImage wi);
		void generateVocabulary();

		void setVocabulary(const WriterVocabulary voc);
		WriterVocabulary vocabulary() const;
		void saveVocabulary(QString filePath);

		void evaluateDatabase(QStringList classLabels, QStringList filePaths, QString filePath = QString());
		void evaluateDatabase(cv::Mat hists, QStringList classLabels, QStringList filePaths, QString filePath = QString()) const;

		void writeCompetitionEvaluationFile(QStringList imageNames, QString outputPath) const;
		void writeCompetitionEvaluationFile(cv::Mat hists, QStringList imageNames, QString outputPath) const;

	private:
		QString debugName() const;
		cv::Mat calculatePCA(const cv::Mat desc, bool normalizeBefore = false);
		void generateBOW(cv::Mat desc);
		void generateGMM(cv::Mat desc);
		void writeMatToFile(const cv::Mat, const QString filePath) const;
		void loadFeatures(const QString filePath, cv::Mat& descriptors, QVector<cv::KeyPoint>& keypoints);
		QVector<QVector<cv::KeyPoint> > mKeyPoints;
		QVector<cv::Mat> mDescriptors;
		WriterVocabulary mVocabulary = WriterVocabulary();
	};
}
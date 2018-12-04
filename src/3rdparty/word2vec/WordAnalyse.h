/*******************************************************************************************************
Code is from Qindazhu, see
https://github.com/qindazhu/word2vec

The code is distributed  WITHOUT ANY WARRANTY. The source code owner is the owner of the following github profile: https://github.com/qindazhu
No license for thw files are specified at the repository.
*******************************************************************************************************/

#pragma once

#include <cassert>
#include <vector>
#include <string>
#include <unordered_map>
#include <queue>
#include <algorithm>
#include <utility>
#include <numeric>
#include <chrono>
#include <fstream>
#include <sstream>
#include <iostream>

typedef float real;                    // Precision of float numbers
typedef std::vector<real> Vector;

class WordAnalyse
{
public:
	bool hasWord(const std::string word) const { return vocab_.find(word) != vocab_.end(); }

	WordAnalyse(){}

	~WordAnalyse(){
		for (auto& w : words_) {
			delete w;
		}
	}

	void loadVectors(const std::string& file) {
		std::cout << "load vectors..." << std::endl;
		std::ifstream in(file);
		int words_num, word_vec_size;
		in >> words_num >> word_vec_size;
		for (size_t i = 0; i < words_num; ++i) {
			// "word freq"
			int word_count;
			std::string word_text;
			in >> word_text >> word_count;
			// vector
			real val;
			Vector word_vec(word_vec_size);
			for (size_t n = 0; n < word_vec_size; ++n) {
				in >> val;
				word_vec[n] = val;
			}
			// normalize 
			// http://stats.stackexchange.com/questions/177905/should-i-normalize-word2vecs-word-vectors-before-using-them
			// For cosine similarity: normalize
			//  find analogies : It's ok to no normalization(so use difference of vectors) or with normalization(so use difference of direction of vectors)
			// if the downstream applications are able to (or need to) consider more sensible aspects, 
			// such as word significance, or consistency in word usage (see below), then normalization might not be such a good idea.
			//normVector(word_vec);
			Word* word = new Word(word_text, word_count);
			word->vec_.swap(word_vec);
			words_.push_back(word);
			vocab_[word_text] = i;
		}
	}

	struct NearestWord {
		std::string text_;
		real distance_;

		NearestWord(std::string text, real distance) : text_(text), distance_(distance) {}

		struct cmp {
			bool operator()(const NearestWord& ls, const NearestWord& rs) const {
				return ls.distance_ < rs.distance_;
			}
		};
	};

	// nearest top-N neigbors
	void getNNWords(const std::string& single_word, std::vector<NearestWord>& result, size_t topN = 20) {
		std::cout << "compute top-" << topN << " similar words of word " << single_word << std::endl;
		if (!hasWord(single_word)) {
			return;
		}
		size_t single_word_index = vocab_[single_word];
		Vector single_word_vec = words_[single_word_index]->vec_;
		getNN(single_word_vec, single_word_index, topN, result);
		
	}

	// word1 - word2 = word3 - result[i] 
	// result[i] != word2
	void wordAnalogy(const std::string& word1, const std::string& word2, const std::string& word3, std::vector<NearestWord>& result, size_t topN = 20) {
		if (!hasWord(word1) || !hasWord(word2) || !hasWord(word3)) {
			return;
		}
		// word3 - word1 + word2
		Vector v1 = words_[vocab_[word1]]->vec_;
		Vector v2 = words_[vocab_[word2]]->vec_;
		Vector v3 = words_[vocab_[word3]]->vec_;
		Vector sim_vec(v1.size());
		for (size_t i = 0; i < v1.size(); ++i) {
			sim_vec[i] = v3[i] - v1[i] + v2[i];
		}
		normVector(sim_vec);
		size_t single_word_index = vocab_[word2]; // result[i] != word2
		getNN(sim_vec, single_word_index, topN, result);
	}

private:
	struct Word {
		std::string text_;
		uint32_t count_; 
		Vector vec_;

		Word(std::string text, uint32_t count) : text_(text), count_(count){}
		Word(const Word&) = delete;
		const Word& operator = (const Word&) = delete;

		struct cmp {
			bool operator()(const Word& w1, const Word& w2) const {
				return w1.count_ > w2.count_;
			}
		};
	};

	std::vector<Word*> words_;
	std::unordered_map<std::string, size_t> vocab_; // <word, index-in-words_>

	void getNN(Vector& single_word_vec, size_t single_word_index, size_t topN, std::vector<NearestWord>& result) {
		topN = std::max((size_t)0, std::min(topN, words_.size() - 1)); // exclude the singleWord self
		std::priority_queue<NearestWord, std::vector<NearestWord>, NearestWord::cmp> distance_heap;
		size_t i = 0;
		for (; i < topN + 1; ++i) {
			if (i == single_word_index) {
				continue;
			}
			real d = distanceFun(single_word_vec, words_[i]->vec_);
			distance_heap.push(NearestWord(words_[i]->text_, d));
		}
		if (distance_heap.size() > topN) {
			distance_heap.pop(); // exclude the redundant one 
		}
		for (; i < words_.size(); ++i) {
			if (i == single_word_index) {
				continue;
			}
			real d = distanceFun(single_word_vec, words_[i]->vec_);
			if (d < distance_heap.top().distance_) {
				distance_heap.pop();
				distance_heap.push(NearestWord(words_[i]->text_, d));
			}
		}
		result.clear();  // guarantee result is empty
		for (size_t n = 0; n < topN; ++n) {
			result.push_back(distance_heap.top());
			distance_heap.pop();
		}
	}

	real distanceFun(Vector& v1, Vector& v2) {
		assert(v1.size() == v2.size());
		real d = 0;
		// consine distance
		for (size_t i = 0; i < v1.size(); ++i) {
			d += v1[i] * v2[i];
		}
		//return fabs(d);
		return d;
	}

	void normVector(Vector& v) {
		real norm = 0.0;
		for (size_t i = 0; i < v.size(); ++i) {
			norm += v[i] * v[i];
		}
		norm = sqrt(norm);
		for (size_t i = 0; i < v.size(); ++i) {
			v[i] /= norm;
		}
	}
};
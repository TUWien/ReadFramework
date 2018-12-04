/*******************************************************************************************************
Code is from Qindazhu, see
https://github.com/qindazhu/word2vec

The code is distributed  WITHOUT ANY WARRANTY. The source code owner is the owner of the following github profile: https://github.com/qindazhu
No license for thw files are specified at the repository.
*******************************************************************************************************/

#pragma once
#include <cassert>
#include <vector>
#include <map>
#include <list>
#include <string>
#include <unordered_map>
#include <queue>
#include <deque>
#include <tuple>
#include <algorithm>
#include <utility>
#include <numeric>
#include <chrono>
#include <random>
#include <memory>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iterator>

#pragma warning(push, 0)	// no warnings from includes
// Qt Includes
#include <QObject>
#include <QMap>
#include <QVector>
#include <QString>
#include <QVariantMap>
#pragma warning(pop)



typedef float real;                    // Precision of float numbers
typedef std::vector<real> Vector;
typedef std::vector<std::string> Sentence;

class Word2Vec
{
public:
	bool hasWord(const std::string word) const { return vocab_.find(word) != vocab_.end(); }

	Word2Vec(int size = 200, int window = 5, float sample = 0.001, uint32_t min_count = 7, bool cbow = false,
		bool hs = true, int negative = 1, float starting_alpha = 0.025, float min_alpha = 0.0001*0.025, int debug_mode = 2)
		:layer1_size_(size), window_size_(window), sample_(sample), min_count_(min_count), cbow_(cbow),
		hs_(hs), negative_(negative), starting_alpha_(starting_alpha), min_alpha_(min_alpha), debug_mode_(debug_mode), alpha_(starting_alpha)
	{}

	~Word2Vec()
	{
		for (auto& w : words_) {
			delete w;
		}
	}

	size_t buildVocab(const std::string& file) {
		printInfo(1, "build vocab...");
		std::ifstream in(file);
		std::string word;
		int i = 0;
		while (in >> word) {
			++vocab_[word];
			++i;
			if (i % 10000 == 0) {
				printInfo(2, std::to_string(i) + " words has been processed...");
			}
		}
		// reduce word
		for (auto it = vocab_.begin(); it != vocab_.end();) {
			if (it->second < min_count_) {
				it = vocab_.erase(it);
			}
			else {
				++it;
			}
		}
		return vocab_.size();
	}

	size_t buildVocabFromWords(std::vector<std::string>& wordsI) {
		printInfo(1, "build vocab...");
		std::string word;
		int i = 0;
		for(std::vector<std::string>::iterator it = wordsI.begin(); it != wordsI.end(); ++it) {
			++vocab_[*it];
			++i;
			if (i % 10000 == 0) {
				printInfo(2, std::to_string(i) + " words has been processed...");
			}
		}
		// reduce word
		for (auto it = vocab_.begin(); it != vocab_.end();) {
			if (it->second < min_count_) {
				it = vocab_.erase(it);
			}
			else {
				++it;
			}
		}

		return vocab_.size();
	}

	size_t buildVocabFromVocab(QMap<QString, int>& vocab) {
		printInfo(1, "build vocab...");
		std::string word;
		int i = 0;

		QMap<QString, int>::iterator it = vocab.begin();
		while (it != vocab.end()) {
			++i;
			
			QString test = it.key();
			vocab_[it.key().toStdString()] = it.value();
			if (i % 10000 == 0) {
				printInfo(2, std::to_string(i) + " words has been processed...");
			}
			++it;
		}

		return vocab_.size();
	}


	void readTrainWords(std::vector<std::string>& wordsI) {
		printInfo(1, "load training sentences...");
		//std::string word;
		int i = 0;
		Sentence s;
		std::default_random_engine eng(uint32_t(::time(NULL)));
		std::uniform_real_distribution<double> distribution(0.0, 1.0);
		//while (in >> word) {
		for (std::vector<std::string>::iterator itW = wordsI.begin(); itW != wordsI.end(); ++itW) {
			
			auto it = vocab_.find(*itW);
			if (it == vocab_.end()) {
				continue;
			}
			// subsampling
			if (sample_ > 0) {
				double word_count = it->second;
				// sqrt(t/f(w)) + t/f(w) = (sqrt(f(w)/t) + 1)*(t/f(w))
				double r = (sqrt(word_count / (sample_ * total_words_count_)) + 1) * (sample_ * total_words_count_) / word_count;
				if (r < distribution(eng)) {
					continue;
				}
			}
			s.push_back(*itW);
			if (s.size() > MAX_SENTENCE_LENGTH) {
				sentences_.push_back(s);
				s.clear();
			}
			++i;
			if (i % 10000 == 0) {
				printInfo(2, std::to_string(i) + " words has been loaded...");
			}
		}
	}


	void readTrainWords(const std::string& file) {
		printInfo(1, "load training sentences...");
		std::ifstream in(file);
		std::string word;
		int i = 0;
		Sentence s;
		std::default_random_engine eng(uint32_t(::time(NULL)));
		std::uniform_real_distribution<double> distribution(0.0, 1.0);
		while (in >> word) {
			auto it = vocab_.find(word);
			if (it == vocab_.end()) {
				continue;
			}
			// subsampling
			if (sample_ > 0) {
				double word_count = it->second;
				// sqrt(t/f(w)) + t/f(w) = (sqrt(f(w)/t) + 1)*(t/f(w))
				double r = (sqrt(word_count / (sample_ * total_words_count_)) + 1) * (sample_ * total_words_count_) / word_count;
				if (r < distribution(eng)) {
					continue;
				}
			}
			s.push_back(word);
			if (s.size() > MAX_SENTENCE_LENGTH) {
				sentences_.push_back(s);
				s.clear();
			}
			++i;
			if (i % 10000 == 0) {
				printInfo(2, std::to_string(i) + " words has been loaded...");
			}
		}
	}

	void train() {
		printInfo(1, "training begin...");
		for (auto& x : sentences_) {
			trainSentence(x);
		}
	}

	void initNet() {
		// word index map
		createHuffmanTree();
		initExpTable();
		initUnigramTable();
		buildWordIndex();

		// init net params
		size_t vocab_size = words_.size();
		net0_.resize(vocab_size);
		std::default_random_engine eng(uint32_t(::time(NULL)));
		std::uniform_real_distribution<real> distribution(0.0, 1.0);
		for (auto& s : net0_) {
			s.resize(layer1_size_);
			for (auto& x : s) x = real((distribution(eng) - 0.5) / layer1_size_);
		}
		net1hs_.resize(vocab_size);
		for (auto& s : net1hs_) {
			s.resize(layer1_size_);
		}
		if (negative_) {
			net1neg_.resize(vocab_size);
			for (auto& s : net1neg_) {
				s.resize(layer1_size_);
			}
		}
		// init words count
		last_trained_words_count_ = 0;
		trained_words_count_ = 0;
		total_words_count_ = std::accumulate(words_.begin(), words_.end(), 0,
			[](uint32_t memo, Word* &word) { return memo + word->count_; });
	}

	void saveVectors(const std::string& file) {
		printInfo(1, "save word vectors...");
		std::ofstream out(file);
		//out << net0_.size() << " " << net0_[0].size() << std::endl;
		for (size_t i = 0; i < words_.size(); ++i) {
			// "word freq"
			out << words_[i]->text_ << " " << words_[i]->count_ << std::endl;
			// vector
			std::copy(net0_[i].begin(), net0_[i].end(), std::ostream_iterator<real>(out, " "));
			out << std::endl;
		}
	}

	void saveVectorsMap(QVariantMap& vecMap) {
		printInfo(1, "save word vectors to json...");
		//out << net0_.size() << " " << net0_[0].size() << std::endl;
		for (size_t i = 0; i < words_.size(); ++i) {
			// "word freq"

			QVector<real> vec = QVector<real>::fromStdVector(net0_[i]);
			QString vecStr;
			for (auto v : vec) {
				vecStr += QString::number((double)v);
				vecStr += " ";
			}
			vecMap.insert(QString::fromStdString(words_[i]->text_), vecStr);

			//out << words_[i]->text_ << " " << words_[i]->count_ << std::endl;
			// vector
			//std::copy(net0_[i].begin(), net0_[i].end(), std::ostream_iterator<real>(out, " "));
			//out << std::endl;
		}
	}

private:
	struct Word
	{
		int64_t index_; // word index in vocab table
		std::string text_;
		uint32_t count_; // word count in corpus
		Word *left_, *right_;

		std::vector<uint8_t> codes_; // huffman-tree path code from root to current word node
		std::vector<int64_t> points_; // the index of nodes from root to current word node

		Word(int64_t index, std::string text, uint32_t count, Word *left = nullptr, Word *right = nullptr) : index_(index), text_(text), count_(count), left_(left), right_(right) {}
		Word(const Word&) = delete;
		const Word& operator = (const Word&) = delete;

		struct cmp {
			bool operator()(const Word* w1, const Word* w2) const {
				return w1->count_ > w2->count_;
			}
		};
	};

	// params
	const int MAX_SENTENCE_LENGTH = 1000;
	const uint64_t UNIGRAM_TABLE_SIZE = 100000000; //1e8
	const size_t EXP_TABLE_SIZE = 1000;
	const real MAX_EXP = 6.0;

	int layer1_size_; // word vec dims
	int window_size_;
	float sample_; // the param t of subsampling
	uint32_t min_count_; // dropout word whose freq is less than min_count_
	float alpha_;
	float min_alpha_;
	float starting_alpha_;
	// 0: no info output
	// 1: main stage
	// 2: intermediate info
	int debug_mode_;

	uint64_t total_words_count_;
	uint64_t trained_words_count_; // count of words which has been used for training until now
	uint64_t last_trained_words_count_;

	bool cbow_; // use cbow architecture if true, else use skip-gram

	std::vector<Vector> net0_; // word vecotr need to learn

	// hierarchical softmax
	bool hs_;
	std::vector<Vector> net1hs_; // params vector in hierarchical softmax approach, weights of non-leaf nodes of huffman tree

	// negative sampling
	int negative_;
	std::vector<Vector> net1neg_; // params vector in negative sampling approach.

	std::unordered_map<std::string, uint32_t> vocab_; // <word, freq>
	std::vector<Word*> words_;
	// TODO: redundant, can be moved, such as reuse vocab_?
	std::unordered_map<std::string, size_t> words_index_;
	std::vector<size_t> unigram_;  // unigram table: sampling word from the vocab according to words' freq
	std::vector<real> exp_table_;
	std::vector<Sentence> sentences_;

	void createHuffmanTree() {
		printInfo(1, "    create huffman Tree...");
		std::priority_queue<Word*, std::vector<Word*>, Word::cmp> word_heap;
		for (auto it = vocab_.begin(); it != vocab_.end(); ++it) {
			Word* temp = new Word(0, it->first, it->second); // set word->count = 0 since it is useless for building huffman tree.
			word_heap.push(temp);
		}
		// build huffman tree
		int64_t index = 0; // index node which is not leaf node, then construct points using this index
		while (word_heap.size() > 1) {
			++index;
			Word* w1 = word_heap.top();
			word_heap.pop();
			Word* w2 = word_heap.top();
			word_heap.pop();
			// left node with smaller freq
			if (w1->count_ > w2->count_) {
				std::swap(w1, w2);
			}
			Word* temp = new Word(index, "", w1->count_ + w2->count_, w1, w2);
			word_heap.push(temp);
		}
		// build words_ with codes_ and points
		Word* root = word_heap.top();
		word_heap.pop();
		words_.reserve(vocab_.size());
		// traverse tree level by level
		std::deque<std::pair<Word*, int> > q; // <Word*, level>
		q.push_back(std::make_pair(root, 0));
		while (!q.empty()) {
			root = q.front().first;
			int currLevel = q.front().second;
			q.pop_front();
			// Huffman tree is always a full tree(either no children or two children), no need to check right node
			if (root->left_ == nullptr) {
				words_.push_back(root);
				continue;
			}
			// assign codec 0 to left node, 1 to right node
			root->left_->codes_ = root->codes_;
			root->left_->codes_.push_back(0);
			root->right_->codes_ = root->codes_;
			root->right_->codes_.push_back(1);
			// update points
			root->points_.push_back(index - root->index_);
			root->left_->points_ = root->points_;
			root->right_->points_ = root->points_;
			q.push_back(std::make_pair(root->left_, currLevel + 1));
			q.push_back(std::make_pair(root->right_, currLevel + 1));
			root->left_ = nullptr;
			root->right_ = nullptr;
			delete root;
		}
	}

	// init sampling table
	void initUnigramTable() {
		unigram_.resize(UNIGRAM_TABLE_SIZE);
		const double power = 0.75;
		double train_words_pow = std::accumulate(words_.begin(), words_.end(), 0.0, [power](double memo, Word* &word) {return memo + ::pow(word->count_, power); });
		size_t index = 0;
		double cdf = ::pow(words_[index]->count_, power) / train_words_pow;
		size_t vocab_size = words_.size();
		for (size_t i = 0; i < UNIGRAM_TABLE_SIZE; ++i) {
			unigram_[i] = index;
			if (i / (double)UNIGRAM_TABLE_SIZE > cdf) {
				++index;
				if (index >= vocab_size) {  // it happens only if cdf_sum is less than 1.0 since precision loss
					index = vocab_size - 1;
					while (i < UNIGRAM_TABLE_SIZE) {
						unigram_[i] = index;
						++i;
					}
					break;
				}
				else {
					cdf += ::pow(words_[index]->count_, power) / train_words_pow;
				}

			}

		}
	}

	// precompute sigmoid table
	void initExpTable() {
		exp_table_.resize(EXP_TABLE_SIZE + 1);
		for (size_t i = 0; i < exp_table_.size(); ++i) {
			exp_table_[i] = exp((i / (real)EXP_TABLE_SIZE) * 2 - 1) * MAX_EXP;
			exp_table_[i] = exp_table_[i] / (1 + exp_table_[i]);
		}
	}

	void buildWordIndex() {
		for (size_t i = 0; i < words_.size(); ++i) {
			words_index_[words_[i]->text_] = i;
		}
	}

	void trainSentence(const Sentence& sentence) {
		// init net parmas of hidden layer
		Vector cbow_input_sum_neu1(layer1_size_, 0);
		Vector cbow_update_of_input(layer1_size_, 0);
		Vector skip_gram_update_of_input(layer1_size_, 0);
		// update learning rate
		trained_words_count_ += sentence.size();
		if (trained_words_count_ - last_trained_words_count_ > 10000) {
			alpha_ = std::max(min_alpha_, real(starting_alpha_ * (1.0 - 1.0 * trained_words_count_ / total_words_count_)));
			printInfo(2, std::to_string(trained_words_count_) + " words has been trained...");
		}
		std::default_random_engine eng(uint32_t(::time(NULL)));
		std::uniform_int_distribution<int> distribution(0); // for reduce window size 
		int sentenc_len = (int)sentence.size();
		for (int word_pos = 0; word_pos < sentenc_len; ++word_pos) {
			size_t curr_word_index = words_index_[sentence[word_pos]];
			const Word* curr_word = words_[curr_word_index];
			Vector& curr_word_vector = net0_[curr_word_index];
			// window for context
			int reduce_window_size = distribution(eng) % window_size_;
			int min_pos = std::max(0, word_pos - window_size_ + reduce_window_size);
			int max_pos = std::min(sentenc_len - 1, word_pos + window_size_ - reduce_window_size);

			if (cbow_) {
				std::fill(cbow_input_sum_neu1.begin(), cbow_input_sum_neu1.end(), (real)0);
				std::fill(cbow_update_of_input.begin(), cbow_update_of_input.end(), (real)0);
				// Input to Hidden: sum vector of context words
				for (int pos = min_pos; pos <= max_pos; ++pos) {
					if (pos == word_pos) {
						continue;
					}
					Vector& context_word = net0_[words_index_[sentence[pos]]];
					addVector(cbow_input_sum_neu1, context_word);
					//std::transform(cbow_input_sum_neu1.begin(), cbow_input_sum_neu1.end(), context_word.begin(), cbow_input_sum_neu1.begin(), std::plus<real>());
				}
				// hierarchical softmax
				if (hs_) {
					for (size_t level = 0; level < curr_word->codes_.size(); ++level) {
						// Hidden to Output:
						Vector& curr_level_predictor = net1hs_[curr_word->points_[level]];
						real f = std::inner_product(cbow_input_sum_neu1.begin(), cbow_input_sum_neu1.end(), curr_level_predictor.begin(), (real)0);
						if (f > MAX_EXP || f < -MAX_EXP) {
							continue;
						}
						else {
							f = exp_table_[int((f + MAX_EXP) * (EXP_TABLE_SIZE / MAX_EXP / 2.0))];
						}
						// g is predict error by the learning rate
						real g = (1 - curr_word->codes_[level] - f) * alpha_;
						// Propagate errors output -> hidden
						updateVector(cbow_update_of_input, curr_level_predictor, g);
						// Learn predictor weights 
						updateVector(curr_level_predictor, cbow_input_sum_neu1, g);
					}
				}
				// negative sampling
				if (negative_ > 0) {
					for (size_t sample_idx = 0; sample_idx < negative_ + 1; ++sample_idx) {
						size_t target = curr_word_index;
						int label = 1;
						if (sample_idx != 0) {
							label = 0;
							target = unigram_[distribution(eng) % UNIGRAM_TABLE_SIZE];
							// TODO: why
							if (target == 0) {
								target = distribution(eng) % (words_.size() - 1) + 1;
							}
							if (target == curr_word_index) {
								continue;
							}
						}
						Vector& curr_negative_sample_predictor = net1neg_[target];
						real f = std::inner_product(cbow_input_sum_neu1.begin(), cbow_input_sum_neu1.end(), curr_negative_sample_predictor.begin(), (real)0);
						if (f > MAX_EXP) {
							f = 1;
						}
						else if (f < -MAX_EXP) {
							f = 0;
						}
						else {
							f = exp_table_[int((f + MAX_EXP) * (EXP_TABLE_SIZE / MAX_EXP / 2.0))];
						}
						// g is predict error by the learning rate
						real g = (label - f) * alpha_;
						// Propagate errors output -> hidden
						updateVector(cbow_update_of_input, curr_negative_sample_predictor, g);
						// Learn predictor weights 
						updateVector(curr_negative_sample_predictor, cbow_input_sum_neu1, g);
					}
				}
				// Hidden -> In: update word vector
				for (int pos = min_pos; pos <= max_pos; ++pos) {
					if (pos == word_pos) {
						continue;
					}
					Vector& context_word = net0_[words_index_[sentence[pos]]];
					addVector(context_word, cbow_update_of_input);
				}

			}
			else {
				// skip-gram
				for (int pos = min_pos; pos <= max_pos; ++pos) {
					if (pos == word_pos) {
						continue;
					}
					Vector& context_word = net0_[words_index_[sentence[pos]]];
					std::fill(skip_gram_update_of_input.begin(), skip_gram_update_of_input.end(), (real)0);
					// hierarchical softmax
					if (hs_) {
						// symmetry property: 
						//    if one context_word W_u is in the window around of  curr_word W_w, then
						//    W_w is also in the window around of W_u,
						//    so we use v(W_u)*theta(W_w) here instead of v(W_w)*theta(W_u)
						for (size_t level = 0; level < curr_word->codes_.size(); ++level) {
							// Hidden to Output:
							Vector& curr_level_predictor = net1hs_[curr_word->points_[level]];
							real f = std::inner_product(context_word.begin(), context_word.end(), curr_level_predictor.begin(), (real)0);
							if (f > MAX_EXP || f < -MAX_EXP) {
								continue;
							}
							else {
								f = exp_table_[int((f + MAX_EXP) * (EXP_TABLE_SIZE / MAX_EXP / 2.0))];
							}
							// g is predict error by the learning rate
							real g = (1 - curr_word->codes_[level] - f) * alpha_;
							// Propagate errors output -> hidden
							updateVector(skip_gram_update_of_input, curr_level_predictor, g);
							// Learn predictor weights 
							updateVector(curr_level_predictor, context_word, g);
						}
					}
					// negative sampling
					if (negative_ > 0) {
						for (size_t sample_idx = 0; sample_idx < negative_ + 1; ++sample_idx) {
							size_t target = curr_word_index;
							int label = 1;
							if (sample_idx != 0) {
								label = 0;
								target = unigram_[distribution(eng) % UNIGRAM_TABLE_SIZE];
								// TODO: why
								if (target == 0) {
									target = distribution(eng) % (words_.size() - 1) + 1;
								}
								if (target == curr_word_index) {
									continue;
								}
							}
							Vector& curr_negative_sample_predictor = net1neg_[target];
							real f = std::inner_product(context_word.begin(), context_word.end(), curr_negative_sample_predictor.begin(), (real)0);
							if (f > MAX_EXP) {
								f = 1;
							}
							else if (f < -MAX_EXP) {
								f = 0;
							}
							else {
								f = exp_table_[int((f + MAX_EXP) * (EXP_TABLE_SIZE / MAX_EXP / 2.0))];
							}
							// g is predict error by the learning rate
							real g = (label - f) * alpha_;
							// Propagate errors output -> hidden
							updateVector(skip_gram_update_of_input, curr_negative_sample_predictor, g);
							// Learn predictor weights 
							updateVector(curr_negative_sample_predictor, context_word, g);
						}
					}
					// Hidden -> In: update word vector
					addVector(context_word, skip_gram_update_of_input);
				}
			}
		}
	}

	void addVector(Vector& dst, Vector& src) {
		assert(dst.size() == src.size());
		for (size_t i = 0; i < dst.size(); ++i) {
			dst[i] += src[i];
		}
	}

	void updateVector(Vector& dst, Vector& src, real gradient) {
		assert(dst.size() == src.size());
		for (size_t i = 0; i < dst.size(); ++i) {
			dst[i] += gradient * src[i];
		}
	}

	void printInfo(int debug_mode_threshold, const std::string& info) {
		if (debug_mode_ >= debug_mode_threshold) {
			std::cout << info << std::endl;
		}
	}
};


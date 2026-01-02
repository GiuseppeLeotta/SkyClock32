#ifndef U_RANDOM_H
#define U_RANDOM_H


// uRandom.cpp
// Classe per ESP8266 che genera numeri randomici unici da 0 a n (incluso), come nella tombola.
// Ora supporta l'impostazione del massimo n tramite funzione setMax().

#include <Arduino.h>

class uRandom {
public:
    /**
     * Costruisce un generatore vuoto. È necessario chiamare setMax() prima di next().
     */
    uRandom()
      : _maxv(0), _count(0), _remaining(0), _pool(nullptr)
    {}

    ~uRandom() {
        if (_pool) free(_pool);
    }

    /**
     * Imposta il massimo valore (inclusivo) e resetta il pool.
     * @param maxValue valore massimo da 0 a maxValue
     */
    void setMax(uint16_t maxValue) {
        // libera vecchio pool
        if (_pool) {
            free(_pool);
        }
        _maxv = maxValue;
        _count = _maxv + 1;
        // alloca nuovo pool
        _pool = (uint16_t*)malloc(_count * sizeof(uint16_t));
        reset();
    }

    /**
     * Estrae un numero casuale non ancora usato.
     * @return numero [0..max], oppure -1 se esauriti o massimo non impostato
     */
    int16_t next() {
        if (!_pool || _remaining == 0) {
            return -1; // segnale di esaurimento o non inizializzato
        }
        uint16_t idx = random(_remaining);
        uint16_t value = _pool[idx];
        // swap con ultimo disponibile
        _pool[idx] = _pool[_remaining - 1];
        _pool[_remaining - 1] = value;
        _remaining--;
        return value;
    }

    /**
     * Verifica se ci sono ancora numeri da estrarre
     */
    bool hasNext() const {
        return (_pool && _remaining > 0);
    }

    /**
     * Ripristina lo stato iniziale del pool
     */
    void reset() {
        if (!_pool) return;
        for (uint16_t i = 0; i <= _maxv; i++) {
            _pool[i] = i;
        }
        _remaining = _count;
    }

    /**
     * Totale valori nel pool (= max+1)
     */
    uint16_t size() const {
        return _count;
    }

    /**
     * Numeri rimasti da estrarre
     */
    uint16_t remaining() const {
        return _remaining;
    }

private:
    uint16_t _maxv;
    uint16_t _count;
    uint16_t _remaining;
    uint16_t* _pool;
};

#endif

import {
  RANKS,
  SUITS,
  getBackImage,
  getDisplayRank,
  getSuitClass,
  getSuitSymbol,
} from '../lib/cards'
import type { CardCode, CardValue } from '../lib/cards'

type CardPickerModalProps = {
  isOpen: boolean
  usedCards: CardCode[]
  onClose: () => void
  onSelect: (value: CardValue) => void
}

export default function CardPickerModal({
  isOpen,
  usedCards,
  onClose,
  onSelect,
}: CardPickerModalProps) {
  if (!isOpen) return null

  return (
    <div className="modal-overlay" onClick={onClose}>
      <div
        className="card-picker-modal compact"
        onClick={(e) => e.stopPropagation()}
      >
        <div className="picker-top-row">
          <button
            type="button"
            className="back-card-button"
            onClick={() => onSelect('BACK')}
            title="Face down"
          >
            <img
              src={getBackImage()}
              alt="Face down card"
              className="back-card-image"
            />
          </button>
        </div>

        <div className="card-picker-grid compact-grid">
          {SUITS.map((suit) => (
            <div key={suit} className="card-picker-row compact-row">
              {RANKS.map((rank) => {
                const card = `${rank}${suit}` as CardCode
                const disabled = usedCards.includes(card)

                return (
                  <button
                    key={card}
                    type="button"
                    className={`rank-suit-button ${getSuitClass(suit)} ${
                      disabled ? 'disabled' : ''
                    }`}
                    onClick={() => onSelect(card)}
                    disabled={disabled}
                    title={card}
                  >
                    <span className="rank-text">{getDisplayRank(rank)}</span>
                    <span className="suit-text">{getSuitSymbol(suit)}</span>
                  </button>
                )
              })}
            </div>
          ))}
        </div>
      </div>
    </div>
  )
}
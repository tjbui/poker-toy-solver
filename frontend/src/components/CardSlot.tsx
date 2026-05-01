import type { CardValue, Rank, Suit } from '../lib/cards'
import {
  getBackImage,
  getCardRank,
  getCardSuit,
  getDisplayRank,
  getSuitClass,
  getSuitSymbol,
} from '../lib/cards'

type CardSlotProps = {
  value: CardValue
  onClick: () => void
}

export default function CardSlot({ value, onClick }: CardSlotProps) {
  if (value === null) {
    return (
      <button className="card-slot" onClick={onClick} type="button">
        <div className="empty-card-slot">+</div>
      </button>
    )
  }

  if (value === 'BACK') {
    return (
      <button className="card-slot" onClick={onClick} type="button">
        <img
          src={getBackImage()}
          className="card-image back-image"
          alt="Face down card"
        />
      </button>
    )
  }

  const rank = getCardRank(value) as Rank
  const suit = getCardSuit(value) as Suit

  return (
    <button className="card-slot face-up-card" onClick={onClick} type="button">
      <div className={`card-face ${getSuitClass(suit)}`}>
        <div className="corner top">
          <div>{getDisplayRank(rank)}</div>
          <div>{getSuitSymbol(suit)}</div>
        </div>

        <div className="center-suit">{getSuitSymbol(suit)}</div>

        <div className="corner bottom">
          <div>{getDisplayRank(rank)}</div>
          <div>{getSuitSymbol(suit)}</div>
        </div>
      </div>
    </button>
  )
}
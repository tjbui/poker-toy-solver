import type { CardValue, Rank, Suit } from './cards'
import {
  getBackImage,
  getDisplayRank,
  getSuitClass,
  getSuitSymbol,
} from './cards'

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
        <img src={getBackImage()} className="card-image back-image" alt="Face down card" />
      </button>
    )
  }

  const [suit, rank] = value.split('-') as [Suit, Rank]

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
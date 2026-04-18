import CardSlot from './CardSlot'
import type { CardValue } from '../types/cards'

type CardRowProps = {
  cards: CardValue[]
  onCardClick: (index: number) => void
}

export default function CardRow({ cards, onCardClick }: CardRowProps) {
  return (
    <div className="card-row">
      {cards.map((card, index) => (
        <CardSlot key={index} value={card} onClick={() => onCardClick(index)} />
      ))}
    </div>
  )
}